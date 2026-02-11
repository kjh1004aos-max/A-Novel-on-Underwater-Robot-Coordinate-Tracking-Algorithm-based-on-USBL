using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;

public class Data : MonoBehaviour
{
    public FishData fishData;

    [ContextMenu("To Json Data")]
    void SaveFishDataToJson()
    {
        string jsonData = JsonUtility.ToJson(fishData, true);
        string path = Path.Combine(Application.dataPath, "fishData.json");
        File.WriteAllText(path, jsonData);
    }
    [ContextMenu("From Json Data")]
    void LoadFishDataToJson()
    {
        string path = Path.Combine(Application.dataPath, "fishData.json");
        string jsonData = File.ReadAllText(path);
        fishData = JsonUtility.FromJson<FishData>(jsonData);
    }

    [System.Serializable]
    public class FishData
    {
        public string ID;
        public float version;
        public float BatChar;
    }
}
