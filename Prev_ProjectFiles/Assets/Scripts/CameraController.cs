using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraController : MonoBehaviour
{
    //카메라 회전 속도
    public float rotateSpeed = 10.0f;
    //줌 속도
    public float zoomSpeed = 10.0f;

    public Camera CameraArm;
    //수조 비추는 카메라 Transform 변수
    public Transform mainCamera;
    public Transform fish;
    public BoxCollider2D frame;
    private UIManager Instance;

    void Start()
    {
        frame = GetComponent<BoxCollider2D>();
    }
    void Update()
    {
        Find_Obj();
    }
    //UI스크립트 불러오는 함수
    public void Find_Obj()
    {
        if(GameObject.Find("UI") != null)
            Instance = GameObject.Find("UI").GetComponent<UIManager>();
    }
    //마우스 휠로 수조를 확대/축소하는 함수
    public void Zoom()
    {
        float distance = Input.GetAxis("Mouse ScrollWheel") * -1 * zoomSpeed;
        if (distance != 0)
        {
            CameraArm.fieldOfView += distance;
        }
    }
    //마우스 클릭 드래그로 수조를 회전시키는 함수
    public void Rotate()
    {
        if (Input.GetMouseButton(0))   // 마우스 추가
        {
            Vector2 mouseDelta = new Vector2(Input.GetAxis("Mouse X"), Input.GetAxis("Mouse Y"));
            Vector3 rot = mainCamera.rotation.eulerAngles; // 현재 카메라의 각도를 Vector3로 반환
            float x = rot.x - mouseDelta.y;

            if (x < 180f)
            {
                x = Mathf.Clamp(x, -1f, 70f);
            }
            else
            {
                x = Mathf.Clamp(x, 335f, 361f);
            }
            mainCamera.rotation = Quaternion.Euler(x, rot.y + mouseDelta.x, rot.z);
            if (Instance.point_Obj_List.Count > 0)
            {
                for (int i = 0; i < Instance.cnt; i++)
                {
                    Instance.point_Obj_List[i].GetComponent<Transform>().rotation = Quaternion.Euler(x, rot.y + mouseDelta.x, rot.z);
                }
            }
            if (Instance.traking_Obj_List.Count > 0)
            {
                for (int i = 0; i < Instance.lineCnt; i++)
                {
                    Instance.traking_Obj_List[i].GetComponent<Transform>().rotation = Quaternion.Euler(x, rot.y + mouseDelta.x, rot.z);
                }
            }
        }
    }

}

