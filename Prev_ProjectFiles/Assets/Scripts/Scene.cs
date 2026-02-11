using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

public class Scene : MonoBehaviour
{
    private UIManager UI;

    private void Awake()
    {
        UI = GameObject.Find("UI").GetComponent<UIManager>();
    }
    //씬 전환
    public void Monitoring()
    {
        SceneManager.LoadScene("Monitoring");
        if (GameObject.Find("File Name Dropdown") != null)
        {
            Debug.Log(UI.lineCnt);
            UI.remove_Setpostion();
            UI.Reset_Track_Run();
            for (int i = 0; i < UI.lineCnt; i++)
            {
                
                Destroy(UI.traking_Obj_List[i]);
            }
        }
    }
    public void MainScene()
    {
        SceneManager.LoadScene("Main");
        if (GameObject.Find("File Name Dropdown") != null)
        {
            UI.remove_Setpostion();
            UI.Reset_Track_Run();
            for (int i = 0; i < UI.lineCnt; i++)
            {
                Destroy(UI.traking_Obj_List[i]);
            }
        };
    }
    public void TrackEditor()
    {
        SceneManager.LoadScene("Track Editor");
        if (GameObject.Find("File Name Dropdown") != null)
        {
            UI.remove_Setpostion();
            UI.Reset_Track_Run();
            for (int i = 0; i < UI.lineCnt; i++)
            {
                Destroy(UI.traking_Obj_List[i]);
            }
        }
    }
    public void TrackRun()
    {
        SceneManager.LoadScene("Track Run");
        if (GameObject.Find("File Name Dropdown") != null)
        {
            UI.remove_Setpostion();
            UI.Reset_Track_Run();
            for (int i = 0; i < UI.lineCnt; i++)
            {
                Destroy(UI.traking_Obj_List[i]);
            }
        }
    }
}

