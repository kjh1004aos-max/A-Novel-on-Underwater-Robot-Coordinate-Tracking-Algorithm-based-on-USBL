using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class CheckInFrame : MonoBehaviour, IPointerEnterHandler, IPointerExitHandler, IPointerClickHandler, IPointerDownHandler
{
    bool isOnFrame = false;
    bool isClickFrame = false;

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
    //마우스 포인터가 수조 디스플레이 화면에 들어왔는지 확인하는 함수
    public void OnPointerEnter(PointerEventData eventData)
    {
        isOnFrame = true;
    }
    //마우스 포인터가 수조 디스플레이 화면에 들어왔는지 확인하는 함수
    public void OnPointerExit(PointerEventData eventData)
    {
        isOnFrame = false;
    }
    //마우스 포인터가 수조 디스플레이 화면 클릭중인지 확인하는 함수
    public void OnPointerClick(PointerEventData eventData)
    {
        Debug.Log("디클릭");
        isClickFrame = false;
    }
    //마우스 포인터가 수조 디스플레이 화면 클릭했는지 확인하는 함수
    public void OnPointerDown(PointerEventData eventData)
    {
        Debug.Log("클릭함");
        isClickFrame = true;
    }
}
