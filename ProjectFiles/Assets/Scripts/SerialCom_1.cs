using System;
using System.IO.Ports;
using UnityEngine;
using UnityEngine.UI;
using System.Text.RegularExpressions;
using System.Text;
using System.IO;
using System.Linq;
using System.Collections;
using System.Collections.Generic;

public class SerialCom_1 : MonoBehaviour
{
    //경로 지정 변수
    public static SerialCom_1 Instance;

    //씨리얼 통신을 하기 위한 변수
    private SerialPort serial_150;

    //List 변수
    //포트 번호 리스트
    private List<string> Port = new List<string>();
    //엑셀 파일에 저장된 item들을 담는 리스트
    private List<List<float>> temp_traking_item = new List<List<float>>();
    //좌표 표시 GameObject 리스트
    public List<GameObject> traking_Obj_List = new List<GameObject>();
    //X110
    public List<Vector3> traking_Obj_Vector = new List<Vector3>();
    //좌표 텍스트 리스트
    public List<TextMesh> traking_Text_List = new List<TextMesh>();
    //좌표 연결 라인 리스트
    public List<LineRenderer> traking_Line = new List<LineRenderer>();
    //X, Y, Z를 하나의 좌표로 하는 리스트
    public List<List<float>> traking_wayPoints = new List<List<float>>();

    //드롭다운으로 포트번호 선택을 위한 변수
    public Dropdown portdropdown;
      
    [SerializeField]
    //유영체 나타내는 GameObject 변수
    private GameObject X110;
    //Content에 내용 입력하는 InputField 변수
    private InputField content_input;
    //Contect에 입력되는 문자열 변수
    private String content_txt;
    //경고 문구 나타내는 text 변수
    private Text warning_txt;
    //시간 나타내는 text 변수
    public Text con_Time;
    //포트번호 나타내는 text 변수
    public Text Port_Label;

    //X110 정보 배열
    public string[] slavePos = new string[3];
    //함수에서 유영체의 위치를 파악하기 위한 값을 담는 배열
    public string[] maindatas = new string[3];
    //전달받는 메시지 변수
    public string message = "";
    //buadrate 변수
    public string baudRate = "";
    //fish id 변수
    public string fish_id;
    //포트번호 드롭다운 인덱스 변수
    public int port_Num;
    //기능 선택 드롭다운 인덱스 변수
    public int category_Num;
    //연결 시간 초 변수
    public float con_Sec;
    //연결 시간 분 변수
    public int con_Min;

    //Traking 기능 실행시 
    //실행한 시점부터 총 시간
    public float temp_traking_time;
    //실행한 시점부터 초
    public float temp_traking_sec;
    //실행한 시점부터 분
    public float temp_traking_min;
    //시간 카운팅 스위치 변수
    public bool time_switch = false;
    //좌표 위치 Vector 값
    private Vector3 temp_traking_point_vector;
    //현재 좌표, 이전 좌표 위치 Vector 값
    private Vector3 before_Vector, cur_Vector;

    private Vector3 pos;

    public float x150_yaw = 0;
     
    public int cnt = 0;

    private void Awake()
    {
        if (null == Instance)
        {
            Instance = this;
            DontDestroyOnLoad(this.gameObject);
        }
        else
        {
            Destroy(this.gameObject);
        }
    }
    void Update()
    {
        Get_Port();
        InputMessage();
        Click_Connect_Button();
        Click_SendPing_Button();
        Click_Traking_Button();
        Click_X110_Traking_Reset_Button();
        Check_Connect();
        if (GameObject.Find("NoX_150") == null)
            Get_Fish_Position();
        if (time_switch)
            Temp_Timer();
    }
    //포트 드롭다운 할당 함수
    private void Get_Port()
    {
        if (GameObject.Find("Port Select") != null)
        {
            portdropdown = GameObject.Find("Port Select").GetComponent<Dropdown>();
            AddPortList();
        }
    }
    //연결 버튼 실행
    private void Click_Connect_Button()
    {
        if (GameObject.Find("Connect Button") != null)
        { 
            if (GameObject.Find("Connect Txt").GetComponent<Text>().text == "Disconnect")
            {
                GameObject.Find("Connect Button").GetComponent<Button>().onClick.RemoveListener(Connect_Port);
                GameObject.Find("Connect Button").GetComponent<Button>().onClick.RemoveListener(Disconnect_Port);
                GameObject.Find("Connect Button").GetComponent<Button>().onClick.AddListener(Disconnect_Port);
            }
            else if (GameObject.Find("Connect Txt").GetComponent<Text>().text == "Connect")
            {
                GameObject.Find("Connect Button").GetComponent<Button>().onClick.RemoveListener(Connect_Port);
                GameObject.Find("Connect Button").GetComponent<Button>().onClick.RemoveListener(Disconnect_Port);
                GameObject.Find("Connect Button").GetComponent<Button>().onClick.AddListener(Connect_Port);
            }
        }
    }
    //트래킹 초기화 버튼 실행
    private void Click_X110_Traking_Reset_Button()
    {
        if (GameObject.Find("Reset Traking Button") != null)
        {
            GameObject.Find("Reset Traking Button").GetComponent<Button>().onClick.RemoveListener(X110_Track_Reset);
            GameObject.Find("Reset Traking Button").GetComponent<Button>().onClick.AddListener(X110_Track_Reset);
        }
    }
    //포트번호 목록 추가
    private void AddPortList()
    {
        portdropdown.ClearOptions();
        Port.Clear();
        string[] activ_port = SerialPort.GetPortNames();
        Port.Add("Port");
        for (int i = 0; i < activ_port.Length; i++)
        {
            Port.Add(activ_port[i]);
        }

        portdropdown.AddOptions(Port);
        portdropdown.onValueChanged.RemoveListener(Select_Port);
        portdropdown.onValueChanged.AddListener(Select_Port);

        if (GameObject.Find("Port Label") != null)
        {
            Port_Label = GameObject.Find("Port Label").GetComponent<Text>();
            Port_Label.text = portdropdown.options[port_Num].text;
        }
    }
    //선택된 포트번호 변경하는 함수
    public void Select_Port(int value)
    {
        port_Num = portdropdown.value;
    }
    //연결 시간 계산
    public void Timer()
    {
        con_Sec += Time.deltaTime;
        if (con_Sec > 60)
        {
            con_Sec = 0;
            con_Min++;
        }
        if (GameObject.Find("Connect Time Txt") != null)
            GameObject.Find("Connect Time Txt").GetComponent<Text>().text = string.Format("{0:D2}:{1:D2}", con_Min, (int)con_Sec);
    }
    //포트 연결
    private void Connect_Port()
    {
        openSerial();
        Debug.Log("1");
        //portdropdown.DataSource = SerialPort.GetPortNames();       
    }
    //포트 연결 해제
    private void Disconnect_Port()
    {
        serial_150.Close();
        GameObject.Find("Connect Txt").GetComponent<Text>().text = "Connect";
    }
    private void Timer_Reset()
    {
        con_Sec = 0;
    }
    //시리얼 포트 여는 함수
    public void openSerial()
    {
        string par = Port_Label.text;
        try
        {
            serial_150 = new SerialPort(par, int.Parse(baudRate), Parity.None, 8, StopBits.Two);
            serial_150.Open();
            serial_150.ReadTimeout = 1000;
        }
        catch (Exception e)
        {
            Debug.Log(e);
        }
    }
    //send 버튼 클릭시 기능 할당  ping 선택 시 sendPing 함수, User 선택 시 SendUserMsg 함수, Traking 선택시 SendTraking 함수 실행
    private void Click_SendPing_Button()
    {
        if (GameObject.Find("Send Ping Button") != null)
        {
            if (category_Num == 1)
            {
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendUserMsg);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendPing);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendTraking);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.AddListener(SendPing);
            }
            else if (category_Num == 2)
            {
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendPing);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendUserMsg);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendTraking);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.AddListener(SendUserMsg);
            }
            else if (category_Num == 3)
            {
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendTraking);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendPing);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.RemoveListener(SendUserMsg);
                GameObject.Find("Send Ping Button").GetComponent<Button>().onClick.AddListener(SendTraking);
            }
        }
    }
    //Traking 버튼 클릭 시 실시간 위치 추적 다시 클릭하면 위치추적 중지하는 함수 
    private void Click_Traking_Button()
    {
        if (GameObject.Find("Traking Button") != null)
        {
            if (GameObject.Find("Traking Txt").GetComponent<Text>().text == "Traking")
            {
                GameObject.Find("Traking Button").GetComponent<Button>().onClick.RemoveListener(Stop_Traking);
                GameObject.Find("Traking Button").GetComponent<Button>().onClick.RemoveListener(Traking);
                GameObject.Find("Traking Button").GetComponent<Button>().onClick.AddListener(Traking);
            }
            else if (GameObject.Find("Traking Txt").GetComponent<Text>().text == "Stop")
            {
                GameObject.Find("Traking Button").GetComponent<Button>().onClick.RemoveListener(Traking);
                GameObject.Find("Traking Button").GetComponent<Button>().onClick.RemoveListener(Stop_Traking);
                GameObject.Find("Traking Button").GetComponent<Button>().onClick.AddListener(Stop_Traking);
            }
        }
    }
    //0.1초 간격으로 Temp_SendTraking 함수 실행하는 함수
    private void SendTraking()
    {
        time_switch = true;
        InvokeRepeating("Temp_SendTraking", 0f, 0.1f);
    }
    //2초 간격으로 핑 신호를 보내고, 받은 위치 정보를 3D 좌표에 표시하는 함수
    private void Traking()
    {
        InvokeRepeating("SendPing", 0f, 2f);
        InvokeRepeating("X110_Track", 0f, 2f);
        GameObject.Find("Traking Txt").GetComponent<Text>().text = "Stop";
    }
    //실시간 위치 추적 중지하는 함수
    private void Stop_Traking()
    {
        CancelInvoke("SendPing");
        CancelInvoke("X110_Track");
        GameObject.Find("Traking Txt").GetComponent<Text>().text = "Traking";
    }
    //사용자 메시지 입력 함수
    public void InputMessage()
    {
        if (GameObject.Find("Content InputField") != null)
        {
            content_input = GameObject.Find("Content InputField").GetComponent<InputField>();
            content_txt = content_input.text;
        }
    }
    //버튼 클릭 시 사용자 메시지 전송하는 함수
    public void SendUserMsg()
    {
        string sendData = "", getdata, slice_str, PLLength_str, Checksum;
        string cust_data = content_txt;
        int trsnum;
        char one_val;
        //cust_data = "12345678901234567890";
        for (int i = 0; i < cust_data.Length; i++)
        {
            slice_str = cust_data.Substring(i, 1);
            one_val = char.Parse(slice_str);
            trsnum = Convert.ToInt32(one_val);  //여기가 asc.dec로 변환.
            getdata = "" + Convert.ToString(trsnum, 16);    // asc.hex로 변환            
            sendData += getdata;
        }
        PLLength_str = Convert.ToString(cust_data.Length, 16);
        if (PLLength_str.Length < 2)
        {
            PLLength_str = "0" + PLLength_str;
        }
        Debug.Log(PLLength_str);
        string a = "600101" + PLLength_str + sendData;
        Debug.Log(a);
        Checksum = GET_CHECKSUM(a);
        sendData = "#" + a + Checksum + "\n";
        Debug.Log(sendData);
        SendMsg(sendData);
        content_input.text = "";

        //return sendData;
    }
    //체크섬 함수
    public string GET_CHECKSUM(string cust_data)
    {
        ushort poly = 0xA001;
        ushort crc = 0;
        byte[] buf = new byte[cust_data.Length / 2];

        for (int n = 0, m = 0; n < cust_data.Length; n += 2, m++)
        {
            buf[m] = Convert.ToByte(cust_data.Substring(n, 2), 16);
        }

        for (ushort b = 0; b < buf.Length; b++)
        {
            byte v = buf[b];
            for (byte i = 0; i < 8; i++)
            {
                if ((v & 0x01) != (crc & 0x01))
                {
                    crc >>= 1;
                    crc ^= poly;
                }
                else
                {
                    crc >>= 1;
                }
                v >>= 1;
            }
        }
        string hex1 = crc.ToString("X2");
        string swap1 = hex1.Substring(0, 2);
        string swap2 = hex1.Substring(2, 2);
        hex1 = swap2 + swap1;
        Debug.Log("CRC 값 :" + hex1);

        return hex1;
    }
    //버튼 클릭 시 핑신호를 전송하는 함수.
    public void SendPing()
    {
        SendMsg("#4001040187\n");
    }
    //인자로 받은 문자열을 X150으로 송신하는 함수.
    public void SendMsg(string msg)
    {
        try
        {
            if (serial_150.IsOpen)
            {
                serial_150.Write(msg);
                GameObject.Find("Connect Txt").GetComponent<Text>().text = "Disconnect";
                Timer();
            }
        }
        catch (Exception e)
        {
            Debug.Log("x150이 연결되지 않았음");
        }
    }
    //타이머 함수 
    public void Temp_Timer()
    {
        temp_traking_time += Time.deltaTime;

        temp_traking_sec = (float)Math.Truncate(temp_traking_time * 10) / 10;
        if (temp_traking_sec > 60)
        {
            temp_traking_sec = 0;
            temp_traking_min++;
        }
    }
    //입력한 .csv 파일에서 정보 가져와 좌표 표시해주는 함수
    public void Temp_SendTraking()
    {
        int idx_num = 0;
        int line_cnt=0;
        string traking_file_name = content_txt + ".csv";

        DirectoryInfo di = new DirectoryInfo(@"C:");
        FileInfo[] files = di.GetFiles("*.csv");
        string value = "";
        
        foreach (FileInfo file in files)
        {
            if (file.Name == traking_file_name)
            {
                StreamReader reader = new StreamReader(traking_file_name);
                while (true)
                {
                    value = reader.ReadLine();
                    if (value == null)
                    {
                        break;
                    }
                    string[] arr = value.Split(',');
                    
                    temp_traking_item.Add(new List<float>());
                    for (int j = 0; j < 9; j++)
                    {
                        temp_traking_item[idx_num].Add(float.Parse(arr[j]));
                    }
                    idx_num++;
                    line_cnt++;
                }
                for (int i = 0; i < line_cnt; i++)
                {
                    if (temp_traking_sec == temp_traking_item[i][0])
                    {
                        GameObject.Find("Calc X").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][1] * 10) / 10).ToString(); 
                        GameObject.Find("Calc Y").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][2] * 10) / 10).ToString();
                        GameObject.Find("Calc Z").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][3] * 10) / 10).ToString();
                        GameObject.Find("Roll Val").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][4] * 10) / 10).ToString();
                        GameObject.Find("Pitch Val").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][5] * 10) / 10).ToString();
                        GameObject.Find("Yaw Val").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][6] * 10) / 10).ToString();
                        GameObject.Find("Elevation").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][7] * 10) / 10).ToString();
                        GameObject.Find("Azimuth").GetComponent<Text>().text = (Math.Truncate(temp_traking_item[i][8] * 10) / 10).ToString();

                        float tankScale_Y = GameObject.Find("UI").GetComponent<UIManager>().tankScale_Y;
                        temp_traking_point_vector = new Vector3(temp_traking_item[i][1], -temp_traking_item[i][3] + tankScale_Y / 2f, temp_traking_item[i][2]);
                        X110 = Instantiate(X110, temp_traking_point_vector, Quaternion.identity);

                        traking_wayPoints.Add(new List<float>());
                        traking_wayPoints[cnt].Add(pos.x);
                        traking_wayPoints[cnt].Add(pos.y);
                        traking_wayPoints[cnt].Add(pos.z);

                        traking_Obj_List.Add(X110);
                        traking_Obj_Vector.Add(X110.transform.position);
                        traking_Text_List.Add(X110.GetComponentInChildren<TextMesh>());
                        traking_Line.Add(X110.GetComponent<LineRenderer>());
                        numbering_Point_Text();
                        cnt++;
                    }
                }
                reader.Close();
            }
            else
                value = "파일이 없습니다.";
        }
    }
    //FixPosition 값들을 부르는 함수. 현재는 값의 확인을 위한 함수.
    public void SetFishPos(string msg)
    {
        slavePos[0] = msg.Substring(85, 4);
        slavePos[1] = msg.Substring(89, 4);
        slavePos[2] = msg.Substring(93, 4);

        for (int i = 0; i < 3; i++)
        {
            slavePos[i] = Swapcode16(slavePos[i]);
        }
    }
    //로봇 물고기의 ID를 확인하는 함수. 현재는 비콘의 FixSrcid가 출력되도록 되어 있음.
    public void SetFishID(string msg)
    {
        string ID, IDcode;

        IDcode = msg.Substring(5, 2);
        ID = Convert.ToInt32(IDcode, 16).ToString();
        if (GameObject.Find("Fish ID") != null)
            GameObject.Find("Fish ID").GetComponent<Text>().text = ID;  //로봇물고기 띄워줄 오브젝트
        fish_id = ID;
    }
    // 신호세기를 반환하는 함수.
    public void SetRSSI(string msg)
    {
        string RSScode, RSSvalue, front, rear;
        int decoded;

        RSScode = msg.Substring(31, 4);
        front = RSScode.Substring(0, 2);
        rear = RSScode.Substring(2, 2);
        RSScode = rear + front;

        decoded = Convert.ToInt32(RSScode, 16);
        RSSvalue = ((double)decoded / 10.0).ToString();

        if (GameObject.Find("RSS_value") != null)
            GameObject.Find("RSS_value").GetComponent<Text>().text = RSSvalue;  //수신 세기 띄워줄 오브젝트
    }
    //FixUsblElevation을 반환하는 함수.
    public string GetEle(string msg)
    {
        string elevation;

        elevation = msg.Substring(77, 4);
        elevation = Swapcode16(elevation);
        //elevalue = Convert.ToInt32(elevation, 16) / 10.0;//나중에 쓸때를 대비
        return elevation;
    }
    //FixUsblAzimuth를 반환하는 함수.
    public string GetAzi(string msg)
    {
        string azimuth;

        azimuth = msg.Substring(73, 4);
        azimuth = Swapcode16(azimuth);
        //azivalue getx= Convert.ToInt32(azimuth, 16) / 10.0; //나중에 쓸때를 대비
        return azimuth;
    }
    //FixUsblDistance를 반환하는 함수.
    public string GetDistance(string msg)
    {
        string dist;
        dist = msg.Substring(51, 4);
        dist = Swapcode16(dist);

        //distvalue = Convert.ToInt32(dist, 16) / 10.0;  //나중에 쓸때를 대비
        return dist;
    }

    //받은 메시지의 디코딩을 수행하는 함수. 4자리의 메시지를 받아 수행.
    public string Swapcode16(string msg)
    {
        string front, rear, swaped, tmpt, value;
        int decoded;

        front = msg.Substring(0, 2);
        rear = msg.Substring(2, 2);
        swaped = rear + front;
        if (swaped.Substring(0, 1) == "F")
        {
            //Convert.ToInt32는 숫자의 지정된 문자열 표현을 해당하는 32비트 부호 있는 정수로 변환
            decoded = Convert.ToInt32(swaped, 16);
            tmpt = Convert.ToString(decoded, 2);
            tmpt = tmpt.Replace("0", "T");
            tmpt = tmpt.Replace("1", "0");
            tmpt = tmpt.Replace("T", "1");

            value = "-" + Convert.ToInt32(tmpt, 2) / 10.0;
        }
        else
        {
            decoded = Convert.ToInt32(swaped, 16);
            value = ((double)decoded / 10.0).ToString();
        }
        return value;
    }
    //애플리케이션이 종료되기 전에 모든 게임오브젝트에 전달하는 함수
    void OnApplicationQuit()
    {
        try
        {
            serial_150.Close();
        }
        catch (Exception e)
        {
            Debug.Log(e);
        }
        //RunThread = false;
    }
    //3좌표계로 변환하는 함수
    //double[] Getxyz(string msg, params string[] data) //data[0] : Distance, data[1]: Elevation, data[2]: Azimuth
    double[] Getxyz(params string[] data) //data[0] : Distance, data[1]: Elevation, data[2]: Azimuth
    {
        Vector3 origin = GameObject.Find("UI").GetComponent<UIManager>().Origin;

        double dist = Convert.ToDouble(data[0]);
        double Elev = Convert.ToDouble(data[1]);
        double Azim = Convert.ToDouble(data[2]);// - x150_yaw;
        Debug.Log(x150_yaw);


        double PD = Math.Cos(Elev * Math.PI / 180) * dist;

        double[] getdata = new double[3] {
            -Math.Cos(Azim*Math.PI/180) * PD,                     //x좌표 저장
            -Math.Cos((Math.PI/2) - Azim*Math.PI/180) * PD ,      //y좌표 저장    
            Math.Sin(Elev*Math.PI/180) * dist                    // z좌표 저장
        };

        string[] dispstr = new string[3];

        for (int i = 0; i < 3; i++)
        {
            dispstr[i] = getdata[i].ToString();
        }
        if (GameObject.Find("Calc X") != null)
            GameObject.Find("Calc X").GetComponent<Text>().text = string.Format("{0:0.#}", getdata[0] + origin.x);
        if (GameObject.Find("Calc Z") != null)
            GameObject.Find("Calc Z").GetComponent<Text>().text = string.Format("{0:0.#}", getdata[2] + origin.z);
        if (GameObject.Find("Calc Y") != null)
            GameObject.Find("Calc Y").GetComponent<Text>().text = string.Format("{0:0.#}", getdata[1] + origin.y);

        if (GameObject.Find("Elevation") != null)
            GameObject.Find("Elevation").GetComponent<Text>().text = string.Format("{0:0.#}", Elev);
        if (GameObject.Find("Azimuth") != null)
            GameObject.Find("Azimuth").GetComponent<Text>().text = string.Format("{0:0.#}", Azim);
        if (GameObject.Find("Depth") != null)
            GameObject.Find("Depth").GetComponent<Text>().text = string.Format("{0:0.#}", getdata[1]);

        float tankScale_Y = GameObject.Find("UI").GetComponent<UIManager>().tankScale_Y;
        pos = new Vector3(-(float)getdata[0], -(float)getdata[2] + tankScale_Y / 2f, -(float)getdata[1]);
        //물고기 오브젝트 좌표 이동
        if (GameObject.Find("fish(Clone)") != null)
            GameObject.Find("fish(Clone)").GetComponent<Transform>().position = pos + origin;

        return getdata; //3좌표계 변환 값 반환. (Depth,x,y)
    }
    //Yaw를 반환하는 함수
    public string GetYaw(string msg)
    {
        string Yaw;
        Yaw = msg.Substring(11, 4);
        Yaw = Swapcode16(Yaw);

        if (GameObject.Find("Yaw Val") != null)
            GameObject.Find("Yaw Val").GetComponent<Text>().text = Yaw;

        return Yaw;
    }
    //Pitch를 반환하는 함수
    public string GetPitch(string msg)
    {
        string Pitch;
        Pitch = msg.Substring(15, 4);
        Pitch = Swapcode16(Pitch);

        if (GameObject.Find("Pitch Val") != null)
            GameObject.Find("Pitch Val").GetComponent<Text>().text = Pitch;

        return Pitch;
    }
    //Roll를 반환하는 함수
    public string GetRoll(string msg)
    {
        string Roll;
        Roll = msg.Substring(19, 4);
        Roll = Swapcode16(Roll);

        if (GameObject.Find("Roll Val") != null)
            GameObject.Find("Roll Val").GetComponent<Text>().text = Roll;
        return Roll;
    }
    //전달받은 메시지 값을 출력하는 함수
    public void GetMessage(string msg)
    {
        if (GameObject.Find("Message") != null)
            GameObject.Find("Message").GetComponent<Text>().text = msg;
    }
    //시리얼 통신 활성 여부 확인 나타내는 함수
    private void Check_Connect()
    {
        try
        {
            if (serial_150.IsOpen)
            {
                if (GameObject.Find("NoX_150") != null)
                    GameObject.Find("NoX_150").SetActive(false);      
                //SendMsg("#120000A005\n");
                Debug.Log("연결됨");
            }
            else
            {
                if (GameObject.Find("NoX_150") != null)
                    GameObject.Find("NoX_150").SetActive(true);
                if (GameObject.Find("Connect_Button_txt") != null)
                    GameObject.Find("Connect_Button_txt").GetComponent<Text>().text = "Connect";
            }
        }
        catch (Exception e)
        {
            //Debug.Log("예외처리 발생");
        }
    }
    //전달받은 메시지를 통해 데이터 표시해주는 함수
    public void Get_Fish_Position()
    {
        try
        {
            message = serial_150.ReadLine();
            //메시지의 종류가 RESPONSE_USBL 일 경우
            //42로 시작할 경우 각각의 값들을 출력하고, 유영체 위치 표시
            if (message.Contains("$42"))           
            {
                // 함수에 메시지를 인자로 넘겨 각각 필요한 데이터를 파싱하는 함수들.
                GetRoll(message);
                GetPitch(message);
                GetYaw(message);
                GetMessage(message);
                SetRSSI(message);
                SetFishID(message);

                //SetFishPos(message);

                //아래 세 함수에서 유영체의 위치를 파악하기 위한 값들 추출.
                maindatas[0] = GetDistance(message);
                maindatas[1] = GetEle(message);
                maindatas[2] = GetAzi(message);

                //위에서 추출한 값들을 인자로 넘겨 3좌표계로 값을 반환 하는 함수.
                Getxyz(maindatas);
                //Getxyz(message);

                if (GameObject.Find("fish(Clone)") != null)
                {
                    // 유영체 오브젝트의 각도를 반영.
                    Debug.Log("물고기 있음");
                    GameObject.Find("fish(Clone)").GetComponent<Transform>().rotation = Quaternion.Euler(new Vector3(float.Parse(GetPitch(message)), float.Parse(GetRoll(message)), float.Parse(GetYaw(message))));
                }
            }
            else if (message.Contains("$10"))
            {
                string msg = message.Substring(49, 4);
                msg = Swapcode16(msg);
                x150_yaw = float.Parse(msg);
                if (x150_yaw < 0)
                {
                    x150_yaw = 360 - x150_yaw;
                }
            }
            //43으로 시작할 시 전송 실패 텍스트 출력
            else if (message.Contains("$43"))
            {
                if (GameObject.Find("Warning Txt") != null)
                    warning_txt = GameObject.Find("Warning Txt").GetComponent<Text>();
                warning_txt.text = ("전송 실패.");
                Invoke("Reset_Warning_txt", 2f);
            }
            //63으로 시작할 시 사용자 명령어 전송 실패 텍스트 출력
            else if (message.Contains("$63"))
            {
                if (GameObject.Find("Warning Txt") != null)
                    warning_txt = GameObject.Find("Warning Txt").GetComponent<Text>();
                warning_txt.text = ("사용자 명령어 전송 실패.");
                Invoke("Reset_Warning_txt", 2f);
            }
            else if (message.Contains("$61"))
            {
                GetMessage(message);
            }

        }
        catch (System.TimeoutException e)
        {
            throw;
        }
    }
    private void X110_Track()
    {
        X110 = Instantiate(X110, pos, Quaternion.identity);

        traking_wayPoints.Add(new List<float>());
        traking_wayPoints[cnt].Add(pos.x);
        traking_wayPoints[cnt].Add(pos.y);
        traking_wayPoints[cnt].Add(pos.z);

        traking_Obj_List.Add(X110);
        traking_Obj_Vector.Add(X110.transform.position);
        traking_Text_List.Add(X110.GetComponentInChildren<TextMesh>());
        traking_Line.Add(X110.GetComponent<LineRenderer>());
        numbering_Point_Text();

        cnt++;
    }
    //트래킹 초기화
    private void X110_Track_Reset()
    {
        for (int i = 0; i < cnt; i++)
        {
            Destroy(traking_Obj_List[i]);
        }
        for (int i = 0; i < cnt; i++)
        {
            traking_wayPoints.RemoveAt(traking_wayPoints.Count - 1);
            traking_Line.RemoveAt(traking_Line.Count - 1);
            traking_Text_List.RemoveAt(traking_Text_List.Count - 1);
            traking_Obj_Vector.RemoveAt(traking_Obj_Vector.Count - 1);
            traking_Obj_List.RemoveAt(traking_Obj_List.Count - 1);
        }
        cnt = 0;
        CancelInvoke("Temp_SendTraking");

        GameObject.Find("Calc X").GetComponent<Text>().text = "";
        GameObject.Find("Calc Y").GetComponent<Text>().text = "";
        GameObject.Find("Calc Z").GetComponent<Text>().text = "";
        GameObject.Find("Roll Val").GetComponent<Text>().text = "";
        GameObject.Find("Pitch Val").GetComponent<Text>().text = "";
        GameObject.Find("Yaw Val").GetComponent<Text>().text = "";
        GameObject.Find("Elevation").GetComponent<Text>().text = "";
        GameObject.Find("Azimuth").GetComponent<Text>().text = "";
    }
    //좌표 번호 표시하는 함수
    public void numbering_Point_Text()
    {
        for (int i = 0; i <= cnt; i++)
        {
            traking_Text_List[i] = traking_Obj_List[i].GetComponentInChildren<TextMesh>();
            traking_Obj_List[i].GetComponent<MeshRenderer>().material.color = Color.red;
            if (i!=0)
            {
                traking_Obj_List[i - 1].GetComponent<MeshRenderer>().material.color = Color.black;
            }
            //traking_Text_List[i].text = (i + 1).ToString();
            if (i > 0)
                Draw_Tracking(i);
        }
    }
    //설정 경로 그리는 함수
    private void Draw_Tracking(int i)
    {
        if (traking_Obj_List[i - 1] != null)
        {
            traking_Obj_List[i - 1].transform.localScale = new Vector3(0.05f, 0.05f, 0.05f);
            traking_Line[i - 1] = traking_Obj_List[i - 1].GetComponent<LineRenderer>();
            traking_Line[i - 1].enabled = true;
            Track_Line_Setting(i);
        }
    }
    //설정 경로 점선 세팅
    private void Track_Line_Setting(int i)
    {
        traking_Line[i - 1].startWidth = .05f;
        traking_Line[i - 1].endWidth = .05f;

        before_Vector = traking_Obj_Vector[i - 1];
        cur_Vector = traking_Obj_Vector[i];

        traking_Line[i - 1].SetPosition(0, before_Vector);
        traking_Line[i - 1].SetPosition(1, cur_Vector);
        traking_Line[i - 1].material.mainTextureScale = new Vector2(15, 1);
    }
}