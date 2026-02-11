using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
public class PrintFishStatus : MonoBehaviour
{
    

    float connectTime = 0;

    public Button fish_Button;
    bool buttonclick = false;
    string time;

    private void Awake()
    {
        SerialCom_1 status = GameObject.Find("SerialCom").GetComponent<SerialCom_1>();
    }
    private void Update()
    {
        if(buttonclick == true)
        {
            connectTime += Time.deltaTime;
            int minute = (int)connectTime / 60;

            int second = (int)connectTime - (minute * 60);
            GameObject.Find("Connect_time_txt").GetComponent<Text>().text = minute + ":" + second;
            printFishStatus();
        }    
    }
    public void printFishStatus()
    {
        //GameObject.Find("fish_id_txt").GetComponent<Text>().text = status.fish_id;
        GameObject.Find("Firmware_Version_txt").GetComponent<Text>().text = "Version_1";
        GameObject.Find("Battery_Cur_txt").GetComponent<Text>().text = "100%";
        //GameObject.Find("Transfer_stat_txt").GetComponent<Text>().text = "연결";
        buttonclick = true;
    }
}
