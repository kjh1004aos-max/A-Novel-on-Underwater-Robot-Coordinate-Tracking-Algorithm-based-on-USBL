using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class CheckInController : MonoBehaviour, IPointerEnterHandler, IPointerExitHandler, IPointerClickHandler, IPointerDownHandler
{
    bool isOnFrame = false;
    bool isClickFrame = false;
    // Start is called before the first frame update
    void Start()
    {
    }

    // Update is called once per frame
    void Update()
    {
        if (isOnFrame == true)
        {
            GameObject.Find("Camera Arm").GetComponent<CameraController>().Zoom();
        }
        if (isClickFrame == true)
        {
            GameObject.Find("Camera Arm").GetComponent<CameraController>().Rotate();
        }
    }
    public void OnPointerEnter(PointerEventData eventData)
    {
        Debug.Log("들어옴");
        isOnFrame = true;
    }
    public void OnPointerExit(PointerEventData eventData)
    {
        Debug.Log("나감");
        isOnFrame = false;
    }
    public void OnPointerClick(PointerEventData eventData)
    {
        Debug.Log("디클릭");
        isClickFrame = false;
    }
    public void OnPointerDown(PointerEventData eventData)
    {
        Debug.Log("클릭함");
        isClickFrame = true;
    }
}
