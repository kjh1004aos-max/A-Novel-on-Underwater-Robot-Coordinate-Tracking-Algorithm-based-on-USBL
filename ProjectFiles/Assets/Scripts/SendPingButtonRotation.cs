using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SendPingButtonRotation : MonoBehaviour
{
    private Transform _img;
    public float speed;
    public GameObject refresh;

    private void Start()
    {
        _img = refresh.GetComponent<RectTransform>();
    }
    public void RotateImage()
    {
        _img.Rotate(new Vector3(0, 0, -1 * Time.deltaTime));
    }
}
