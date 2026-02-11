using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ZDrawLine : MonoBehaviour
{
    LineRenderer Ir;
    Vector3 cube1Pos, cube2Pos;
    // Start is called before the first frame update
    void Start()
    {
        Ir = GetComponent<LineRenderer>();
        Ir.startWidth = .05f;
        Ir.endWidth = .05f;

        cube1Pos = gameObject.GetComponent<Transform>().position;
    }

    // Update is called once per frame
    void Update()
    {
        Ir.SetPosition(0, cube1Pos);
        Ir.SetPosition(1, GameObject.Find("Z Line").GetComponent<Transform>().position);
    }
}
