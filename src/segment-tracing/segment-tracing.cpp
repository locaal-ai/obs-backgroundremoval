#include "segment-tracing.h"
#include "segment-tracing-credentials.h"

#include <aws/core/Aws.h>
#include <aws/xray/XRayClient.h>
#include <aws/xray/model/PutTraceSegmentsRequest.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>

#include <obs.h>

Aws::SDKOptions awsOptions;
Aws::XRay::XRayClient* xrayClient;

const char* segment_type_to_string(int segment_type) {
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
  default:
    return "unknown";
  }
}

void send_segment_trace(int segment_type, int info) {
	Aws::XRay::Model::PutTraceSegmentsRequest req;
	// get Unix epoch time in seconds
	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto epoch = now_ms.time_since_epoch();
	// convert to seconds
	auto value = std::chrono::duration_cast<std::chrono::seconds>(epoch);
	// convert to hexadecimals
	std::stringstream ss;
	ss << std::hex << value.count();
	std::string epochHex(ss.str());

	// generate random 24 hexademical digits
	char uuid[25];
	for (int i = 0; i < 24; i++) {
		uuid[i] = "0123456789abcdef"[rand() % 16];
	}
	uuid[24] = 0;

	// generate random 16 hexademical digits
	char idHex[17];
	for (int i = 0; i < 16; i++) {
		idHex[i] = "0123456789abcdef"[rand() % 16];
	}
	idHex[16] = 0;

	obs_data_t* traceData = obs_data_create();
	obs_data_set_string(traceData, "trace_id", (std::string("1-") + epochHex + "-" + uuid).c_str());
	obs_data_set_string(traceData, "name", "com.royshilkrot.obs-backgroundremoval");
	obs_data_set_int(traceData, "start_time", value.count());
	obs_data_set_int(traceData, "end_time", value.count());
	obs_data_set_string(traceData, "id", idHex);
  obs_data_t* metadata = obs_data_create();
  obs_data_set_string(metadata, "segment_type", segment_type_to_string(segment_type));
  obs_data_set_int(metadata, "info", info);
  obs_data_set_obj(traceData, "metadata", metadata);
	const char* traceDataStr = obs_data_get_json(traceData);
	blog(LOG_INFO, "Trace data: %s", traceDataStr);

	req.AddTraceSegmentDocuments(traceDataStr);
	obs_data_release(traceData);

	Aws::XRay::Model::PutTraceSegmentsOutcome response = xrayClient->PutTraceSegments(req);
	if (response.IsSuccess()) {
		blog(LOG_INFO, "Successfully put trace segments");
		auto unprocessedSegments = response.GetResult().GetUnprocessedTraceSegments();
		if (unprocessedSegments.size() > 0) {
			blog(LOG_WARNING, "Unprocessed trace segments:");
			for (auto unprocessedSegment : unprocessedSegments) {
				blog(LOG_WARNING, "  %s %s",
					unprocessedSegment.GetId().c_str(),
					unprocessedSegment.GetErrorCode().c_str());
			}
		}
	} else {
		blog(LOG_WARNING, "Error while putting trace segments: %s %s",
			response.GetError().GetExceptionName().c_str(),
			response.GetError().GetMessage().c_str());
	}
}

void segment_tracing_init(void) {
	Aws::InitAPI(awsOptions);

	Aws::Client::ClientConfiguration config;
	config.region = AWS_REGION;
	xrayClient = new Aws::XRay::XRayClient(
		Aws::Auth::AWSCredentials(AWS_API_ACCESS_KEY_ID, AWS_API_SECRET_KEY),
		config);
}

void segment_tracing_deinit(void) {
  delete xrayClient;
  Aws::ShutdownAPI(awsOptions);
}
