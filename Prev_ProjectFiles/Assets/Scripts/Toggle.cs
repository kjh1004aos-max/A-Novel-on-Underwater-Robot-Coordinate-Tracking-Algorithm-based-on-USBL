using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Toggle : MonoBehaviour
{
    public string ID;
    public float Version;
    public int Battery;

    public Text IDText;
    public Text VersionText;
    public Text BatteryText;

    //물고기 정보 표시
    public void PrintStatus(string ID, float Version, int Battery)
    {
        IDText.text =      "ID       :   "+ ID;
        VersionText.text = "Version  :   "+ Version;
        BatteryText.text = "Battery  :   "+ Battery;
    }



}
