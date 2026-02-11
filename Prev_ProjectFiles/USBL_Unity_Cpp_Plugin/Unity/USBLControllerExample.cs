
using System;
using UnityEngine;

public class USBLControllerExample : MonoBehaviour
{
    [Header("Serial")]
    public string portName = "COM3";
    public int baudRate = 115200;
    public int slaveId = 1;

    [Header("Tracking config")]
    public bool azimuthZeroIsNorth = true;
    public bool elevationPositiveDown = true;
    public bool distanceIsSlantRange = true;
    public int phaseSign = -1;
    public double errorThresholdRatio = 0.05;

    [Header("Control")]
    public Transform destination;
    public double kpYaw = 1.0;
    public double kdYaw = 0.1;
    public double maxSteerDeg = 30.0;
    public double throttle = 0.5;

    [Header("Unity mapping")]
    public float positionScale = 1.0f; // 1 meter = 1 Unity unit by default

    private IntPtr ctx = IntPtr.Zero;
    private USBLPlugin.PoseC pose;
    private USBLPlugin.ControlC ctrl;

    private void Awake()
    {
        ctx = USBLPlugin.usbl_create();

        // Apply configs
        var tcfg = new USBLPlugin.TrackerConfigC
        {
            azimuthZeroIsNorth = azimuthZeroIsNorth ? 1 : 0,
            elevationPositiveDown = elevationPositiveDown ? 1 : 0,
            distanceMode = distanceIsSlantRange ? 0 : 1,
            phaseSign = phaseSign,
            errorThresholdRatio = errorThresholdRatio,
            maxPathPoints = 20000
        };
        USBLPlugin.usbl_set_tracker_config(ctx, ref tcfg);

        var ccfg = new USBLPlugin.ControllerConfigC
        {
            kpYaw = kpYaw,
            kdYaw = kdYaw,
            maxSteerDeg = maxSteerDeg,
            throttle = throttle,
            yawZeroIsNorth = azimuthZeroIsNorth ? 1 : 0
        };
        USBLPlugin.usbl_set_controller_config(ctx, ref ccfg);

        // Open serial
        int ok = USBLPlugin.usbl_open_serial(ctx, portName, baudRate);
        if (ok == 0)
        {
            Debug.LogWarning($"USBL serial open failed for {portName} @ {baudRate}");
        }
    }

    private void OnDestroy()
    {
        if (ctx != IntPtr.Zero)
        {
            USBLPlugin.usbl_close_serial(ctx);
            USBLPlugin.usbl_destroy(ctx);
            ctx = IntPtr.Zero;
        }
    }

    private void Update()
    {
        if (ctx == IntPtr.Zero) return;

        // Read/parse new packets (non-blocking)
        USBLPlugin.usbl_update(ctx);

        // Get latest pose
        if (USBLPlugin.usbl_get_latest_pose(ctx, slaveId, out pose) == 1 && pose.hasPosition == 1)
        {
            // ENU -> Unity: (East, North, Up) -> (X, Z, Y) by default
            Vector3 unityPos = new Vector3(
                (float)pose.positionENU.x,
                (float)pose.positionENU.z,
                (float)pose.positionENU.y
            ) * positionScale;

            transform.position = unityPos;

            // Optional: visualize yaw only (Unity Y axis)
            // If your yaw convention differs, adjust here.
            if (pose.hasAttitude == 1)
            {
                transform.rotation = Quaternion.Euler(0f, (float)pose.yawDeg, 0f);
            }
        }

        // Control toward destination (optional)
        if (destination != null)
        {
            // Set destination in ENU (inverse of the mapping above)
            Vector3 d = destination.position / positionScale;
            double destEast = d.x;
            double destNorth = d.z;
            double destUp = d.y;
            USBLPlugin.usbl_set_destination(ctx, slaveId, destEast, destNorth, destUp);

            // Compute control
            if (USBLPlugin.usbl_compute_control(ctx, slaveId, Time.deltaTime, out ctrl) == 1 && ctrl.valid == 1)
            {
                // Send to beacon (NOTE: template format - adapt to your hardware!)
                USBLPlugin.usbl_send_default_command(ctx, slaveId, ref ctrl);

                // For debugging
                Debug.Log($"USBL ctrl: desiredYaw={ctrl.desiredYawDeg:F1}, err={ctrl.headingErrorDeg:F1}, steer={ctrl.steeringDeg:F1}");
            }
        }
    }
}
