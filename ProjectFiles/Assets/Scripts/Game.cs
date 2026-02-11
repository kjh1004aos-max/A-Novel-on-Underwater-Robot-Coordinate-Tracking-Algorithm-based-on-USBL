using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using UnityEngine.SceneManagement;

public class Game : MonoBehaviour
{
    //물고기 선택시 활성화 할 텍스트 오브젝트

    //물고기 ID를 표시할 텍스트 컴포넌트
    public Text ID;
    //텍스트 버전을 표시할 텍스트 컴포넌트
    public Text Version;
    //배터리 잔량을 표시할 텍스트 컴포넌트
    public Text BatChar;

    public void SaveFishPosition()
    {
        FishData save = new FishData();
        save.x = transform.position.x;
        save.y = transform.position.y;
        save.z = transform.position.z;
        SaveManager.Save(save);
    }
    public void LoadFIshPosition()
    {
        FishData save = SaveManager.Load();
        transform.position = new Vector3(save.x, save.y, save.z);
    }
    public void PrintFishData()
    {

    }
    public void FishReExplore()
    {
        
    }

    public void FishPositionReset()
    {

    }
    public void PathCall()
    {

    }

    public void PathSave()
    {

    }
    public void Play()
    {

    }
    public void Pause()
    {

    }

    public class GameController : MonoBehaviour
    {
        public static float fTime;
        public static int userLevel = 0;

        [Serializable]
        public class PlayerData
        {
            public int userLevel;
            public float fTime;
        }

        void Start()
        {
            LoadData();
        }
        void Update()
        {
            //F5키를 누르면 저장함수 호출
            if (Input.GetKeyDown(KeyCode.F5))
                SaveData();

            //F9키를 누르면 레벨 1씩 증가
            if (Input.GetKeyDown(KeyCode.F9))
                userLevel++;

            //fTime 계속 증가
            fTime += Time.deltaTime;
        }

        public void SaveData()
        {
            BinaryFormatter bf = new BinaryFormatter();
            FileStream file = File.Create(Application.persistentDataPath + "/PlayerInfo.dat");

            PlayerData data = new PlayerData();

            //A --> B 에 할당
            data.userLevel = userLevel;
            data.fTime = fTime;

            //B 직렬화하여 파일에 담기
            bf.Serialize(file, data);
            file.Close();
        }
        public void LoadData()
        {
            BinaryFormatter bf = new BinaryFormatter();
            FileStream file = File.Open(Application.persistentDataPath + "playerInfo.dat", FileMode.Open);

            if(file != null & file.Length > 0)
            {
                //파일 역직렬화하여 B에 담기
                PlayerData data = (PlayerData)bf.Deserialize(file);

                //B-->A에 할당
                userLevel = data.userLevel;
                fTime = data.fTime;
            }
            file.Close();
        }
    }


}
