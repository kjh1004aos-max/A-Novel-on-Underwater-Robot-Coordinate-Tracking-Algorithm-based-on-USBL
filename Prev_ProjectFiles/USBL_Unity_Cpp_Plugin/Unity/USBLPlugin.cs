
using System;
using System.Runtime.InteropServices;
using UnityEngine;

public static class USBLPlugin
{
    // IMPORTANT:
    // - On Windows, the plugin file name is "USBLUnityPlugin.dll"
    // - On macOS, "libUSBLUnityPlugin.dylib"
    // - On Linux, "libUSBLUnityPlugin.so"
    // Unity uses the same import name without extension in DllImport.
    private const string DLL = "USBLUnityPlugin";

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec3C
    {
        public double x;
        public double y;
        public double z;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PoseC
    {
        public Vec3C positionENU;
        public double yawDeg;
        public double rollDeg;
        public double pitchDeg;
        public int hasPosition;
        public int hasAttitude;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ControlC
    {
        public double desiredYawDeg;
        public double headingErrorDeg;
        public double steeringDeg;
        public double throttle;
        public int valid;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TrackerConfigC
    {
        public int azimuthZeroIsNorth;
        public int elevationPositiveDown;
        public int distanceMode; // 0=SlantRange, 1=HorizontalRange
        public int phaseSign;    // +1 or -1
        public double errorThresholdRatio;
        public int maxPathPoints;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ControllerConfigC
    {
        public double kpYaw;
        public double kdYaw;
        public double maxSteerDeg;
        public double throttle;
        public int yawZeroIsNorth;
    }

    [DllImport(DLL)] public static extern IntPtr usbl_create();
    [DllImport(DLL)] public static extern void usbl_destroy(IntPtr ctx);

    [DllImport(DLL)] public static extern int usbl_open_serial(IntPtr ctx, string portName, int baudRate);
    [DllImport(DLL)] public static extern void usbl_close_serial(IntPtr ctx);
    [DllImport(DLL)] public static extern int usbl_is_serial_open(IntPtr ctx);
    [DllImport(DLL)] public static extern int usbl_update(IntPtr ctx);

    [DllImport(DLL)] public static extern int usbl_feed_line(IntPtr ctx, string line);

    [DllImport(DLL)] public static extern void usbl_set_tracker_config(IntPtr ctx, ref TrackerConfigC cfg);
    [DllImport(DLL)] public static extern void usbl_set_controller_config(IntPtr ctx, ref ControllerConfigC cfg);

    [DllImport(DLL)] public static extern int usbl_get_latest_pose(IntPtr ctx, int slaveId, out PoseC outPose);

    [DllImport(DLL)] public static extern void usbl_set_destination(IntPtr ctx, int slaveId, double xENU, double yENU, double zENU);
    [DllImport(DLL)] public static extern int usbl_compute_control(IntPtr ctx, int slaveId, double dtSeconds, out ControlC outControl);

    [DllImport(DLL)] public static extern int usbl_send_default_command(IntPtr ctx, int slaveId, ref ControlC control);
    [DllImport(DLL)] public static extern int usbl_send_raw_ascii(IntPtr ctx, string ascii);

    [DllImport(DLL)] public static extern int usbl_get_last_error(IntPtr ctx, System.Text.StringBuilder outUtf8, int outCapacity);
}
