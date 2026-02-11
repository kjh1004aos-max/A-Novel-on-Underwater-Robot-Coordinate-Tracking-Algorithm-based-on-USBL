
#pragma once
#include <cstdint>

#if defined(_WIN32)
    #if defined(USBL_BUILD_DLL)
        #define USBL_API __declspec(dllexport)
    #else
        #define USBL_API __declspec(dllimport)
    #endif
#else
    #define USBL_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// C ABI for Unity. Keep structs POD and stable.

typedef struct UsbLVec3C {
    double x;
    double y;
    double z;
} UsbLVec3C;

typedef struct UsbLPoseC {
    UsbLVec3C positionENU;
    double yawDeg;
    double rollDeg;
    double pitchDeg;
    int hasPosition;
    int hasAttitude;
} UsbLPoseC;

typedef struct UsbLControlC {
    double desiredYawDeg;
    double headingErrorDeg;
    double steeringDeg;
    double throttle;
    int valid;
} UsbLControlC;

typedef struct UsbLTrackerConfigC {
    int azimuthZeroIsNorth;
    int elevationPositiveDown;
    int distanceMode; // 0=SlantRange, 1=HorizontalRange
    int phaseSign;    // +1 or -1
    double errorThresholdRatio;
    int maxPathPoints;
} UsbLTrackerConfigC;

typedef struct UsbLControllerConfigC {
    double kpYaw;
    double kdYaw;
    double maxSteerDeg;
    double throttle;
    int yawZeroIsNorth;
} UsbLControllerConfigC;

// Lifecycle
USBL_API void* usbl_create();
USBL_API void  usbl_destroy(void* ctx);

// Serial I/O
USBL_API int   usbl_open_serial(void* ctx, const char* portName, int baudRate);
USBL_API void  usbl_close_serial(void* ctx);
USBL_API int   usbl_is_serial_open(void* ctx);
USBL_API int   usbl_update(void* ctx); // returns number of parsed packets

// Testing / simulation: feed a single ASCII packet line (same format as serial).
USBL_API int   usbl_feed_line(void* ctx, const char* line);

// Configuration
USBL_API void  usbl_set_tracker_config(void* ctx, const UsbLTrackerConfigC* cfg);
USBL_API void  usbl_set_controller_config(void* ctx, const UsbLControllerConfigC* cfg);

// State retrieval per slave
USBL_API int   usbl_get_latest_pose(void* ctx, int slaveId, UsbLPoseC* outPose);
USBL_API int   usbl_get_path_points(void* ctx, int slaveId, UsbLVec3C* outPoints, int maxPoints);

// Destination and control
USBL_API void  usbl_set_destination(void* ctx, int slaveId, double xENU, double yENU, double zENU);
USBL_API int   usbl_compute_control(void* ctx, int slaveId, double dtSeconds, UsbLControlC* outControl);

// Command sending
USBL_API int   usbl_send_default_command(void* ctx, int slaveId, const UsbLControlC* control);
USBL_API int   usbl_send_raw_ascii(void* ctx, const char* ascii);

// Diagnostics
USBL_API int   usbl_get_last_error(void* ctx, char* outUtf8, int outCapacity);

#ifdef __cplusplus
} // extern "C"
#endif
