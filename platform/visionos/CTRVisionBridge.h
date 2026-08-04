#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <platform/native_vision.h>

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif
FOUNDATION_EXPORT int CTRNativeMain(
	int argc,
	char * _Nullable * _Nonnull argv
);
#ifdef __cplusplus
}
#endif

@interface CTRVisionFrameHub : NSObject
+ (nullable id<MTLDevice>)device;
+ (nullable id<MTLTexture>)currentSceneTexture;
+ (nullable id<MTLTexture>)currentHudTexture;
+ (uint64_t)serial;
@end

NS_ASSUME_NONNULL_END
