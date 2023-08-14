#include <string>
#include <functional>

#include <Foundation/Foundation.h>

#include <obs.h>
#include <plugin-support.h>

void dispatch(const std::function<void(std::string, int)> &callback,
	      std::string responseBody, int errorCode)
{
	dispatch_async(dispatch_get_main_queue(), ^{
		callback(responseBody, errorCode);
	});
}

void fetchStringFromUrl(const char *urlString,
			std::function<void(std::string, int)> callback)
{
	NSString *urlNsString = [[NSString alloc] initWithUTF8String:urlString];
	NSURLSession *session = [NSURLSession sharedSession];
	NSURL *url = [NSURL URLWithString:urlNsString];
	NSURLSessionDataTask *task = [session
		  dataTaskWithURL:url
		completionHandler:^(NSData *_Nullable data,
				    NSURLResponse *_Nullable response,
				    NSError *_Nullable error) {
			if (error != NULL) {
				obs_log(LOG_INFO, "error");
				dispatch(callback, "", 1);
			} else if (response == NULL) {
				dispatch(callback, "", 2);
			} else if (data == NULL || data.length == 0) {
				dispatch(callback, "", 3);
			} else if (![response isKindOfClass:NSHTTPURLResponse
								    .class]) {
				dispatch(callback, "", 4);
			} else {
				std::string responseBody(
					(const char *)data.bytes, data.length);
				NSHTTPURLResponse *httpResponse =
					(NSHTTPURLResponse *)response;
				NSInteger code = httpResponse.statusCode;
				dispatch(callback, responseBody,
					 code < 200 || code > 299 ? 5 : 0);
			}
		}];
	[task resume];
}
