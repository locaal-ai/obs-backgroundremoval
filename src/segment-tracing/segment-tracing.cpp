#include "segment-tracing.h"
#include "segment-tracing-credentials.h"
#include "SegmentTracingDialog.h"
#include "obs-utils/obs-config-utils.h"
#include "plugin-support.h"

#include <aws/core/Aws.h>
#include <aws/xray/XRayClient.h>
#include <aws/xray/model/PutTraceSegmentsRequest.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>

#include <obs.h>
#include <obs-frontend-api.h>

Aws::SDKOptions awsOptions;
Aws::XRay::XRayClient *xrayClient;
SegmentTracingDialog *dialog;

const char *segment_type_to_string(int segment_type)
{
	switch (segment_type) {
	case SEGMENT_TYPE_PLUGIN_LOAD:
		return "plugin_load";
	case SEGMENT_TYPE_PLUGIN_UNLOAD:
		return "plugin_unload";
	case SEGMENT_TYPE_FILTER_CREATE:
		return "filter_create";
	case SEGMENT_TYPE_FILTER_DESTROY:
		return "filter_destroy";
	case SEGMENT_TYPE_FILTER_UPDATE:
		return "filter_update";
	case SEGMENT_TYPE_FILTER_ACTIVATED:
		return "filter_activated";
	case SEGMENT_TYPE_FILTER_DEACTIVATED:
		return "filter_deactivated";
	case SEGMENT_TYPE_FILTER_USAGE_STATS:
		return "filter_usage_stats";
	case SEGMENT_TYPE_FILTER_ERROR:
		return "filter_error";
	case SEGMENT_TYPE_PLUGIN_INIT_TRACE:
		return "init_trace";
	case SEGMENT_TYPE_FILTER_REMOVE:
		return "filter_remove";
	default:
		return "unknown";
	}
}

static std::string random_string(int len)
{
	std::string str;
	str.resize(len);
	for (int i = 0; i < len; i++) {
		str[i] = "0123456789abcdef"[rand() % 16];
	}
	str[len] = 0;
	return str;
}

enum {
	SEND_SEGMENT_TRACE_SUCCESS = 0,
	SEND_SEGMENT_TRACE_PARTIAL_FAILURE,
	SEND_SEGMENT_TRACE_FAILED,
	SEND_SEGMENT_TRACE_DISABLED,
};

int send_segment_trace(int segment_type, int info)
{
	if (xrayClient == nullptr) {
		return SEND_SEGMENT_TRACE_DISABLED; // tracing is disabled or cannot be initialized
	}

	Aws::XRay::Model::PutTraceSegmentsRequest req;
	// get Unix epoch time in seconds
	const auto now = std::chrono::system_clock::now();
	const auto now_ms =
		std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	const auto epoch = now_ms.time_since_epoch();
	// convert to seconds
	const auto value =
		std::chrono::duration_cast<std::chrono::seconds>(epoch);
	// convert to hexadecimals
	std::stringstream ss;
	ss << std::hex << value.count();
	const std::string epochHex(ss.str());

	// generate random 24 hexademical digits
	const std::string uuid = random_string(24);

	// generate random 16 hexademical digits
	const std::string idHex = random_string(16);

	obs_data_t *traceData = obs_data_create();
	obs_data_set_string(
		traceData, "trace_id",
		(std::string("1-") + epochHex + "-" + uuid).c_str());
	obs_data_set_string(traceData, "name",
			    "com.royshilkrot.obs-backgroundremoval");
	obs_data_set_int(traceData, "start_time", value.count());
	obs_data_set_int(traceData, "end_time", value.count());
	obs_data_set_string(traceData, "id", idHex.c_str());
	obs_data_t *metadata = obs_data_create();
	obs_data_set_string(metadata, "segment_type",
			    segment_type_to_string(segment_type));
	obs_data_set_int(metadata, "info", info);

	obs_data_set_obj(traceData, "metadata", metadata);
	const char *traceDataStr = obs_data_get_json(traceData);

	req.AddTraceSegmentDocuments(traceDataStr);
	obs_data_release(traceData);

	Aws::XRay::Model::PutTraceSegmentsOutcome response =
		xrayClient->PutTraceSegments(req);
	if (response.IsSuccess()) {
		auto unprocessedSegments =
			response.GetResult().GetUnprocessedTraceSegments();
		if (unprocessedSegments.size() > 0) {
			obs_log(LOG_WARNING, "Unprocessed trace segments:");
			for (auto unprocessedSegment : unprocessedSegments) {
				obs_log(LOG_WARNING, "  %s %s",
					unprocessedSegment.GetId().c_str(),
					unprocessedSegment.GetErrorCode()
						.c_str());
			}
			return SEND_SEGMENT_TRACE_PARTIAL_FAILURE;
		}
		return SEND_SEGMENT_TRACE_SUCCESS;
	} else {
		obs_log(LOG_WARNING,
			"Error while putting trace segments: %s %s",
			response.GetError().GetExceptionName().c_str(),
			response.GetError().GetMessage().c_str());
		return SEND_SEGMENT_TRACE_FAILED;
	}
}

void segment_tracing_init(void)
{
	bool showSegmentTracingDialog;
	if (getFlagFromConfig("show_segment_tracing_dialog",
			      &showSegmentTracingDialog) ==
	    OBS_BGREMOVAL_CONFIG_FAIL) {
		showSegmentTracingDialog = true;
	}
	if (showSegmentTracingDialog) {
		segment_tracing_show_dialog();
	}
	segment_tracing_init_tracing();
}

void segment_tracing_init_tracing(void)
{
	obs_log(LOG_INFO, "Initializing segment tracing");

	xrayClient = nullptr;

	bool shouldEnable;
	if (getFlagFromConfig("segment_tracing", &shouldEnable) ==
	    OBS_BGREMOVAL_CONFIG_FAIL) {
		obs_log(LOG_WARNING,
			"Failed to read segment tracing config, cannot enable segment tracing");
		return;
	}
	if (!shouldEnable) {
		obs_log(LOG_INFO, "Segment tracing disabled");
		return;
	}

	Aws::InitAPI(awsOptions);

	Aws::Client::ClientConfiguration config;
	config.region = AWS_REGION;
	xrayClient = new Aws::XRay::XRayClient(
		Aws::Auth::AWSCredentials(AWS_API_ACCESS_KEY_ID,
					  AWS_API_SECRET_KEY),
		config);

	if (send_segment_trace(SEGMENT_TYPE_PLUGIN_INIT_TRACE, 0) !=
	    SEND_SEGMENT_TRACE_SUCCESS) {
		obs_log(LOG_WARNING,
			"Failed to send plugin init trace. Segment tracing will be disabled.");
		delete xrayClient;
		xrayClient = nullptr;
		return;
	}
}

void segment_tracing_deinit(void)
{
	obs_log(LOG_INFO, "Deinitializing segment tracing");
	if (xrayClient != nullptr) {
		delete xrayClient;
		xrayClient = nullptr;
		Aws::ShutdownAPI(awsOptions);
	}
}

void segment_tracing_show_dialog(void)
{
	dialog = new SegmentTracingDialog(
		(QWidget *)obs_frontend_get_main_window());
	QTimer::singleShot(2000, dialog, &SegmentTracingDialog::exec);
}
