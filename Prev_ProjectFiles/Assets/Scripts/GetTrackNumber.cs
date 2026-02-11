using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System.Text.RegularExpressions;
using System.Text;
using System.IO;
using System.Linq;

public class GetTrackNumber : MonoBehaviour
{
    public Dropdown FileNamedropdown;

    private string selectTrack;

    List<string> File_Name = new List<string>();
    // Start is called before the first frame update
    void Start()
    {
        Get_File_Name();
    }

    // Update is called once per frame
    void Update()
    {
        
    }
    public void Get_File_Name()
    {
        DirectoryInfo di = new DirectoryInfo(@"C:\Users\cch\Downloads\Airo");
        FileInfo[] files = di.GetFiles("*.txt");
        string str = "";

        File_Name.Add("트랙");
        foreach (FileInfo file in files)
        {
           // Debug.Log(file.Name);
            File_Name.Add(file.Name.ToString());
        }
        
        FileNamedropdown.options.Clear();
       
        FileNamedropdown.AddOptions(File_Name);

        //Debug.Log(FileNamedropdown.options[FileNamedropdown.value + 1].text);
        //Debug.Log(files.Length);
        //  string path = Application.persistentDataPath + @files;
        //  Debug.Log(path);


        //  string textValue = System.IO.File.ReadAllLines(path);

        // if (files == )
        // {
        //      FileInfo txtfile = new FileInfo();
        // }
        int index = FileNamedropdown.value;

       for(index = 0; index < files.Length; index++)
        {
            Debug.Log(files[index].ToString());
            Debug.Log(FileNamedropdown.options[FileNamedropdown.value + 1].text);

            if (files[index].ToString().Equals(FileNamedropdown.options[FileNamedropdown.value + 1].text))
            {
                Get_Text_Read(files[index].ToString());
            }
        }
    }

    public void Get_Text_Read(string filePath)
    {
        FileInfo fileinfo = new FileInfo(filePath);
        string value = "";

        if (fileinfo.Exists)
        {
            StreamReader reader = new StreamReader(filePath);
            value = reader.ReadToEnd();
            Debug.Log(value);
            reader.Close();
        }
        else
            value = "파일이 없습니다.";
               
    }
}