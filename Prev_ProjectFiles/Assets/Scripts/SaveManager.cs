using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.Serialization.Formatters.Binary;   //직렬화/역직렬화를 사용하기 위한 네임 스페이스
using System.IO;                                        //파일 입출력을 위한 네임 스페이스
using System;
public static class SaveManager 
{
    public static void Save(FishData data)
    {
        BinaryFormatter formatter = new BinaryFormatter();
        string path = Path.Combine(Application.dataPath, "FishPosistionData.dat");      //Path.Combine(a, b)는 a경로와 b경로값을 연결   
                                                                                        //Application.dataPath는 특적 경로값을 반환
        FileStream stream = File.Create(path);

        formatter.Serialize(stream, data);
        stream.Close();
    }
    public static FishData Load()
    {
        try
        {
            BinaryFormatter fomatter = new BinaryFormatter();
            string path = Path.Combine(Application.dataPath, "FishPosistionData.dat");

            FileStream stream = File.OpenRead(path);

            FishData data = (FishData)fomatter.Deserialize(stream);
            stream.Close();
            return data;
        }
        catch(Exception e)
        {
            Debug.Log(e.Message);
            return default;
        }
        
    }
}
