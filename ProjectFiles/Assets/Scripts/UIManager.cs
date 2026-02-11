using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System.Text.RegularExpressions;
using System.Text;
using System.IO;
using System.Linq;
using UnityEngine.SceneManagement;
using UnityEngine.EventSystems;

public class UIManager : MonoBehaviour
{
    //스크립트 인스턴스 변수
    private static UIManager Instance;
    private static SerialCom_1 SerialCom_1;
    //경로 지정 변수
    public string FILEPATH = @"C:\Users\";

    //Dropdown 변수
    //수조 선택 Dropdown 변수
    public Dropdown Tankdropdown;
    //Buadrate 선택 Dropdown 변수
    public Dropdown Baudratedropdown;
    //파일 선택 Dropdown 변수
    public Dropdown FileNamedropdown;
    //기능 선택 Dropdown 변수
    public Dropdown Categorydropdown;

    //GameObject 변수
    //물고기 프리팹 GameObject 변수
    public GameObject fish_obj_prefab;
    //수조 GameObject 변수
    public GameObject RectanglularTank_prefab;
    public GameObject CirclularTank_prefab;
    //수조 벽면 프리팹 GameObject 변수
    public GameObject Floor_prefab;
    //좌표계 표시 프리팹 GameObject 변수
    public GameObject Xline_prefab;
    public GameObject Yline_prefab;
    public GameObject Zline_prefab;
    //포인트 표시 GameObject 변수
    public GameObject point_Obj_prefab;
    //X150 GameObject 프리팹 변수
    public GameObject X150_prefab;
    //X120 GameObject 프리팹 변수
    public GameObject X110_prefab;
    //파일에 저장된 좌표 나타내는 GameObject 프리팹 변수
    public GameObject position_List_Prefab;
    //X150 GameObject 변수
    public GameObject numInterval;
    //X150 재배치 사각형 표시 GameObject 변수
    public GameObject relocate_Frame;
    //X150 재배치 사각형 표시 GameObject 변수
    public GameObject relocate_Box;
    //X150 재배치 원형 표시 GameObject 변수
    public GameObject relocate_Circle;
    //X150 연결여부 아이콘 GameObject 변수
    public GameObject X150_icon;
    //생성된 수조 GameObject 변수
    private GameObject obj;
    //X150 프리팹 GameObject 변수
    private GameObject X150_obj;
    //복제된 X150 GameObject 변수
    private GameObject X150_clone;
    //생성된 수조 벽면 GameObject 변수
    private GameObject floor1;
    private GameObject floor2;
    private GameObject floor3;
    private GameObject floor4;
    private GameObject floor5;
    private GameObject floor6;
    //생성된 좌표계 표시 GameObject 변수
    private GameObject GizmoX;
    private GameObject GizmoY;
    private GameObject GizmoZ;
    //생선된 물고기 GameObject 변수
    private GameObject fish;
    //수조 비추는 카메라 GameObject 변수
    public GameObject Camera;
    //유영체 나타내는 GameObject 변수
    public GameObject X110;


    //InputField 변수
    //수조 크기 입력 InputField 변수
    public InputField inputTankScale_X;
    public InputField inputTankScale_Y;
    public InputField inputTankScale_Z;
    //좌표 입력 InputField 변수
    public InputField inputDirection_X;
    public InputField inputDirection_Y;
    public InputField inputDirection_Z;
    //파일 이름 저장 InputField 변수
    public InputField inputFileName;
    //X150 재배치 입력 InputField 변수
    public InputField input_x150_X_Relo;
    public InputField input_x150_Y_Relo;
    public InputField input_x150_Z_Relo;
    //user메세지 입력 InputField 변수
    public InputField input_content;

    //Text 변수
    //Dropdown에서 선택된 수조 Text 변수
    private Text selected_Watertank;
    //Dropdown에서 선택된 BaudRate Text 변수
    public Text selected_Baud;
    //Dropdown에서 선택된 기능 Text 변수
    private Text selected_Category;
    //Dropdown에서 선택된 파일 Text 변수
    private Text Filename_Label;
    //경고 Text 변수
    public Text warning_txt;

    //Sprite 변수
    //X150 재배치 2D 이미지
    public Sprite circle_Image;
    public Sprite rectangle_Image;

    //Vector 변수
    //X150과 물고기와의 거리 Vector 변수
    public Vector3 dist_vec;
    //X150 재배치 사각형 크기 Vector 변수
    public Vector2 relocate_Box_Size;
    //X150 원점 값
    public Vector3 Origin;
    //좌표 이전, 현재 Vector 변수
    public Vector3 before_Point, cur_Point;
    //좌표 위치 Vector 값
    public Vector3 traking_point_vector;

    //X150 아이콘 클릭여부
    public bool X150_Click;
    public bool file_Change = false;

    //List 변수
    //좌표 표시 GameObject 리스트
    public List<GameObject> point_Obj_List = new List<GameObject>();

    public List<GameObject> file_Point_Obj_List = new List<GameObject>();
    //X, Y, Z를 하나의 좌표로 하는 리스트
    public List<List<float>> WayPoints = new List<List<float>>();
    //
    private List<List<float>> select_file_item = new List<List<float>>();
    //수조탱크 리스트
    List<string> tank = new List<string>();
    //Baud 리스트
    List<string> Baud = new List<string>();
    //카테고리 리스트
    List<string> category = new List<string>();
    //파일 이름 리스트
    List<string> File = new List<string>();
    //저장된 경로 목록 리스트
    List<string> File_Name = new List<string>();
    //파일에 저장된 좌표값 임시 저장 vector 리스트
    List<Vector3> temp_Point_Position = new List<Vector3>();
    //생성된 좌표 리스트
    List<Vector3> point_Position = new List<Vector3>();
    //좌표 텍스트 리스트
    List<TextMesh> point_Text_List = new List<TextMesh>();
    //좌표 연결 라인 리스트
    List<LineRenderer> line = new List<LineRenderer>();

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

    //수조가 생성 여부 bool 변수
    private bool isTank = false;
    //수조탱크 크기 입력값
    public float tankScale_X;
    public float tankScale_Y;
    public float tankScale_Z;
    //수조탱크 크기 임시 저장값
    public float temp_tankScale_X;
    public float temp_tankScale_Y;
    public float temp_tankScale_Z;
    //x150 Relocation 변수값
    public float x150_Relo_X;
    public float x150_Relo_Y;
    public float x150_Relo_Z;
    //x150 Relocation 프레임 변수값
    public float frame_Size_X;
    public float frame_Size_Y;
    //로봇물고기 위치 입력값
    private float direction_X;
    private float direction_Y;
    private float direction_Z;
    //생성된 좌표 개수 변수
    public int cnt = 0;
    //생성된 좌표 연결 라인 개수 변수
    public int lineCnt = 0;
    //수조 모양 인덱스 1은 사각, 2는 원형
    public int index;
    //BuadRate 변수
    public int baud_index;
    //기능 인텍스 1은 현재 위치 표시, 2는 유저 메세지 전송, 3은 엑셀 파일 읽어들이기
    public int category_index;
    //파일 선택 인텍스
    public int file_index;

    private void Awake()
    {
        if (null == Instance)
        {
            Instance = this;

            DontDestroyOnLoad(this.gameObject);
            DontDestroyOnLoad(Camera);
        }
        else
        {
            Destroy(this.gameObject);
        }
    }
    void Update()
    {
        Find_Obj();
        Find_Watertank_Dropdown();
        Find_Baudrate();
        Find_File();
        Find_Category();
        InputDirection();
        InputRelocate();
        InputTankScale();
        Click_Setposition();
        Click_Create_Watertank();
        Click_Remove_Setpostion();
        Click_X150_PopUp();
        Click_Remove_X150_PopUp();
        Click_Set_X150_Locate();
        Click_Set_X150_Default();
        Click_Fish_icon();
        Click_Run_Button();
        Find_Filename();
    }
    //수조 Dropdown 검색
    private void Find_Watertank_Dropdown()
    {
        if (GameObject.Find("Select Water Tank Dropdown") != null)
        {
            Tankdropdown = GameObject.Find("Select Water Tank Dropdown").GetComponent<Dropdown>();
            AddWaterTankList();
        }
    }
    //Baudrate Dropdown 검색
    private void Find_Baudrate()
    {
        if (GameObject.Find("Baud Rate") != null)
        {
            Baudratedropdown = GameObject.Find("Baud Rate").GetComponent<Dropdown>();
            AddBaudRateList();
        }
    }
    //File Name Dropdown 검색
    private void Find_File()
    {
        if (GameObject.Find("File Name Dropdown") != null)
        {
            FileNamedropdown = GameObject.Find("File Name Dropdown").GetComponent<Dropdown>();
            Get_File_Name();
        }
    }
    //송신 Category Dropdown 검색
    private void Find_Category()
    {
        if (GameObject.Find("Category Dropdown") != null)
        {
            Categorydropdown = GameObject.Find("Category Dropdown").GetComponent<Dropdown>();
            AddCategoryList();
        }
    }
    //파일 입력 Inputbar 검색
    private void Find_Filename()
    {
        if (GameObject.Find("File Name Input Bar") != null)
        {
            inputFileName = GameObject.Find("File Name Input Bar").GetComponent<InputField>();
            Click_Save_Track();
        }
    }
    //오브젝트 할당
    private void Find_Obj()
    {
        if (GameObject.Find("SerialCom") != null)
            SerialCom_1 = GameObject.Find("SerialCom").GetComponent<SerialCom_1>();
        if (GameObject.Find("x Scale") != null)
            inputTankScale_X = GameObject.Find("x Scale").GetComponent<InputField>();
        if (GameObject.Find("y Scale") != null)
            inputTankScale_Y = GameObject.Find("y Scale").GetComponent<InputField>();
        if (GameObject.Find("z Scale") != null)
            inputTankScale_Z = GameObject.Find("z Scale").GetComponent<InputField>();
        if (GameObject.Find("x Direction") != null)
            inputDirection_X = GameObject.Find("x Direction").GetComponent<InputField>();
        if (GameObject.Find("y Direction") != null)
            inputDirection_Y = GameObject.Find("y Direction").GetComponent<InputField>();
        if (GameObject.Find("z Direction") != null)
            inputDirection_Z = GameObject.Find("z Direction").GetComponent<InputField>();
        if (GameObject.Find("X150 X InputField") != null)
            input_x150_X_Relo = GameObject.Find("X150 X InputField").GetComponent<InputField>();
        if (GameObject.Find("X150 Y InputField") != null)
            input_x150_Y_Relo = GameObject.Find("X150 Y InputField").GetComponent<InputField>();
        if (GameObject.Find("X150 Z InputField") != null)
            input_x150_Z_Relo = GameObject.Find("X150 Z InputField").GetComponent<InputField>();
        if (GameObject.Find("X150(Clone)") != null)
            X150_clone = GameObject.Find("X150(Clone)");
        if (GameObject.Find("Warning Txt") != null)
            warning_txt = GameObject.Find("Warning Txt").GetComponent<Text>();
        if (GameObject.Find("Relocation Range") != null)
            relocate_Box = GameObject.Find("Relocation Range");
        if (GameObject.Find("Relocation Frame") != null)
            relocate_Frame = GameObject.Find("Relocation Frame");
        if (GameObject.Find("X150") != null)
            X150_icon = GameObject.Find("X150");
        if (GameObject.Find("Content InputField") != null)
            input_content = GameObject.Find("Content InputField").GetComponent<InputField>();
        if (GameObject.Find("Content InputField") != null)
            input_content = GameObject.Find("Content InputField").GetComponent<InputField>();
    }
    //좌표 입력 함수
    public void InputDirection()
    {
        if (GameObject.Find("Direction Input") != null)
        {
            if (float.TryParse(inputDirection_X.text, out direction_X)) { }
            if (float.TryParse(inputDirection_Y.text, out direction_Y)) { }
            if (float.TryParse(inputDirection_Z.text, out direction_Z)) { }
        }
    }
    //수조 크기 입력 함수
    public void InputTankScale()
    {
        if (GameObject.Find("Scale Input") != null)
        {
            if (float.TryParse(inputTankScale_X.text, out tankScale_X)) { }
            if (float.TryParse(inputTankScale_Y.text, out tankScale_Y)) { }
            if (float.TryParse(inputTankScale_Z.text, out tankScale_Z)) { }
        }
    }
    //X150 재배치 입력 합수
    public void InputRelocate()
    {
        if (GameObject.Find("X150 Position") != null)
        {
            if (float.TryParse(input_x150_X_Relo.text, out x150_Relo_X)) { }
            if (float.TryParse(input_x150_Y_Relo.text, out x150_Relo_Y)) { }
        }
    }
    //수조탱크 목록 추가
    private void AddWaterTankList()
    {
        Tankdropdown.ClearOptions();
        tank.Clear();

        tank.Add("수조 모양");
        tank.Add("Rectangle");
        tank.Add("Circle");
        Tankdropdown.AddOptions(tank);

        Tankdropdown.onValueChanged.RemoveListener(SelectWaterTank);
        Tankdropdown.onValueChanged.AddListener(SelectWaterTank);

        if (GameObject.Find("Watertank Label") != null)
        {
            selected_Watertank = GameObject.Find("Watertank Label").GetComponent<Text>();
            selected_Watertank.text = Tankdropdown.options[index].text;
        }
    }
    //BuadRate 목록 추가
    public void AddBaudRateList()
    {
        Baudratedropdown.ClearOptions();
        Baud.Clear();

        Baud.Add("Baud Rate");
        Baud.Add("1200");
        Baud.Add("2400");
        Baud.Add("4800");
        Baud.Add("19200");
        Baud.Add("38400");
        Baud.Add("57600");
        Baud.Add("115200");
        Baudratedropdown.AddOptions(Baud);

        Baudratedropdown.onValueChanged.RemoveListener(Select_Baudrate);
        Baudratedropdown.onValueChanged.AddListener(Select_Baudrate);

        if (GameObject.Find("Baud Label") != null)
        {
            selected_Baud = GameObject.Find("Baud Label").GetComponent<Text>();
            selected_Baud.text = Baudratedropdown.options[baud_index].text;
        }
    }
    //전송 category 목록 추가
    private void AddCategoryList()
    {
        Categorydropdown.ClearOptions();
        category.Clear();

        category.Add("선택");
        category.Add("Ping");
        category.Add("User");
        category.Add("Tracking");

        Categorydropdown.AddOptions(category);

        Categorydropdown.onValueChanged.RemoveListener(Select_Category);
        Categorydropdown.onValueChanged.AddListener(Select_Category);

        if (GameObject.Find("Category Label") != null)
        {
            selected_Category = GameObject.Find("Category Label").GetComponent<Text>();
            selected_Category.text = Categorydropdown.options[category_index].text;
        }
        if (category_index == 1)
        {
            input_content.interactable = false;
        }
        else if (category_index == 2)
        {
            input_content.interactable = true;
        }
        else if (category_index == 3)
        {
            input_content.interactable = true;
        }
        else
        {
            input_content.interactable = false;
        }
    }
    //수조 값 선택
    public void SelectWaterTank(int value)
    {
        index = Tankdropdown.value;
    }
    //BuadRate 값 선택
    public void Select_Baudrate(int value)
    {
        baud_index = Baudratedropdown.value;

        GameObject.Find("SerialCom").GetComponent<SerialCom_1>().baudRate = selected_Baud.text;

    }
    //Category 값 선택
    public void Select_Category(int value)
    {
        category_index = Categorydropdown.value;
        GameObject.Find("SerialCom").GetComponent<SerialCom_1>().category_Num = Categorydropdown.value;
    }
    //수조 생성 클릭
    private void Click_Create_Watertank()
    {
        if (GameObject.Find("Create Watertank Button") != null)
        {
            GameObject.Find("Create Watertank Button").GetComponent<Button>().onClick.RemoveListener(CreateWaterTank);
            GameObject.Find("Create Watertank Button").GetComponent<Button>().onClick.AddListener(CreateWaterTank);
        }
    }
    //X150 재배치 클릭
    private void Click_Set_X150_Locate()
    {
        if (GameObject.Find("X150 Locate Set Button") != null)
        {
            GameObject.Find("X150 Locate Set Button").GetComponent<Button>().onClick.RemoveListener(X150_Relocate);
            GameObject.Find("X150 Locate Set Button").GetComponent<Button>().onClick.AddListener(X150_Relocate);
        }
    }
    //X150 배치 초기화
    private void Click_Set_X150_Default()
    {
        if (GameObject.Find("X150 Postion Default Button") != null)
        {
            GameObject.Find("X150 Postion Default Button").GetComponent<Button>().onClick.RemoveListener(X150_Locate_Default);
            GameObject.Find("X150 Postion Default Button").GetComponent<Button>().onClick.AddListener(X150_Locate_Default);
        }
    }
    //Run 버튼 클릭
    private void Click_Run_Button()
    {
        if (GameObject.Find("Run Button") != null)
        {
            GameObject.Find("Run Button").GetComponent<Button>().onClick.RemoveListener(Run_Track);
            GameObject.Find("Run Button").GetComponent<Button>().onClick.AddListener(Run_Track);
        }
    }
    //물고기 아이콘 클릭시 Display_Fish_Status 함수 실행
    private void Click_Fish_icon()
    {
        if (GameObject.Find("fish_icon") != null)
        {
            GameObject.Find("fish_icon").GetComponent<Button>().onClick.RemoveListener(Display_Fish_Status);
            GameObject.Find("fish_icon").GetComponent<Button>().onClick.AddListener(Display_Fish_Status);
        }
    }
    //Fish Status 텍스트 출력
    private void Display_Fish_Status()
    {
        if (GameObject.Find("Fish ID Input Box") != null)
        {
            GameObject.Find("ID Txt").GetComponent<Text>().text = "Fish - 01";
        }
        if (GameObject.Find("Firmware Version Input Box") != null)
        {
            GameObject.Find("Firmware Version Txt").GetComponent<Text>().text = "1.01";
        }
        if (GameObject.Find("Battery Input Box") != null)
        {
            GameObject.Find("Battery Txt").GetComponent<Text>().text = "100%";
        }
    }
    //카메라 거리 유지
    private void GetDistance()
    {
        if (isTank)
        {
            //Y보다 X의 크기가 더 클 경우 카메라가 기존의 거리보다 X만큼의 거리 뒤에서 관찰
            if (tankScale_X > tankScale_Y)
            {
                GameObject.Find("Camera").GetComponent<Transform>().position = new Vector3(0f, 0f, -(10 + tankScale_X));
            }
            //X보다 Y의 크기가 더 클 경우 카메라가 기존의 거리보다 X만큼의 거리 뒤에서 관찰
            else if (tankScale_X < tankScale_Y)
            {
                GameObject.Find("Camera").GetComponent<Transform>().position = new Vector3(0f, 0f, -(10 + tankScale_Y));
            }
            //X보다 Y의 크기가 같을 경우 카메라가 기존의 거리보다 Y만큼의 거리 뒤에서 관찰
            else
            {
                GameObject.Find("Camera").GetComponent<Transform>().position = new Vector3(0f, 0f, -(10 + tankScale_Y));
            }
        }
    }
    //수조 탱크 생성
    public void CreateWaterTank()
    {
        if (index == 1 && float.Parse(inputTankScale_X.text) > 0 && float.Parse(inputTankScale_Y.text) > 0 && float.Parse(inputTankScale_Z.text) > 0) //사각형 수조
        {
            Destroy(obj);
            Destroy(fish);
            Destroy(floor1);
            Destroy(floor2);
            Destroy(floor3);
            Destroy(floor4);
            Destroy(floor5);
            Destroy(floor6);
            Destroy(GizmoX);
            Destroy(GizmoY);
            Destroy(GizmoZ);
            Destroy(X150_obj);
            fish = Instantiate(fish_obj_prefab);
            X150_obj = Instantiate(X150_prefab, new Vector3(0, tankScale_Z / 2f, 0), Quaternion.identity);

            for (int i = 0; i < cnt; i++)
            {
                Destroy(point_Obj_List[i]);
            }
            for (int i = 0; i < cnt; i++)
            {
                WayPoints.RemoveAt(WayPoints.Count - 1);
                line.RemoveAt(line.Count - 1);
                point_Text_List.RemoveAt(point_Text_List.Count - 1);
                point_Position.RemoveAt(point_Position.Count - 1);
                point_Obj_List.RemoveAt(point_Obj_List.Count - 1);
            }
            cnt = 0;
            warning_txt.text = ("수조가 생성되었습니다.");
            Invoke("Reset_Warning_txt", 2f);
            temp_tankScale_X = tankScale_X;
            temp_tankScale_Y = tankScale_Y;
            temp_tankScale_Z = tankScale_Z;
            //수조 벽면 생성
            floor1 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor1.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Y / 10f);
            floor1.transform.position = new Vector3(0, -tankScale_Z / 2f, 0);

            floor2 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor2.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Y / 10f);
            floor2.transform.position = new Vector3(0, tankScale_Z / 2f, 0);
            floor2.transform.rotation = Quaternion.Euler(new Vector3(0, 0, 180f));

            floor3 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor3.transform.localScale = new Vector3(tankScale_Z / 10f, 1, tankScale_Y / 10f);
            floor3.transform.position = new Vector3(tankScale_X / 2f, 0, 0);
            floor3.transform.rotation = Quaternion.Euler(new Vector3(0, 0, 90f));

            floor4 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor4.transform.localScale = new Vector3(tankScale_Z / 10f, 1, tankScale_Y / 10f);
            floor4.transform.position = new Vector3(-tankScale_X / 2f, 0, 0);
            floor4.transform.rotation = Quaternion.Euler(new Vector3(0, 0, -90f));

            floor5 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor5.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Z / 10f);
            floor5.transform.position = new Vector3(0, 0, tankScale_Y / 2f);
            floor5.transform.rotation = Quaternion.Euler(new Vector3(-90f, 0, 0));

            floor6 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor6.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Z / 10f);
            floor6.transform.position = new Vector3(0, 0, -tankScale_Y / 2f);
            floor6.transform.rotation = Quaternion.Euler(new Vector3(90f, 0, 0));

            //X,Y,Z 라인 생성
            GizmoX = Instantiate(Xline_prefab, Vector3.zero, Quaternion.identity);
            GizmoX.transform.localScale = new Vector3(0.05f, tankScale_X / 2f, 0.05f);
            GizmoX.transform.position = new Vector3(0, -tankScale_Z / 2f, -tankScale_Y / 2f);
            GizmoX.transform.rotation = Quaternion.Euler(new Vector3(90f, 0, 90f));

            GizmoY = Instantiate(Yline_prefab, Vector3.zero, Quaternion.identity);
            GizmoY.transform.localScale = new Vector3(0.05f, tankScale_Y / 2f, 0.05f);
            GizmoY.transform.position = new Vector3(-tankScale_X / 2f, -tankScale_Z / 2f, 0);
            GizmoY.transform.rotation = Quaternion.Euler(new Vector3(0, 90f, 90f));

            GizmoZ = Instantiate(Zline_prefab, Vector3.zero, Quaternion.identity);
            GizmoZ.transform.localScale = new Vector3(0.05f, tankScale_Z / 2f, 0.05f);
            GizmoZ.transform.position = new Vector3(-tankScale_X / 2f, 0, -tankScale_Y / 2f);


            obj = Instantiate(RectanglularTank_prefab, Vector3.zero, Quaternion.identity);
            obj.transform.localScale = new Vector3(tankScale_X, tankScale_Z, tankScale_Y);

            GameObject.Find("Camera Arm").GetComponent<Transform>().rotation = Quaternion.Euler(0, 0, 0);

            Origin = new Vector3(0, tankScale_Z / 2f, 0);
        }
        else if (index == 2 && float.Parse(inputTankScale_X.text) > 0 && float.Parse(inputTankScale_Y.text) > 0 && float.Parse(inputTankScale_Z.text) > 0)    //원통형 수조
        {
            Destroy(obj);
            Destroy(fish);
            Destroy(floor1);
            Destroy(floor2);
            Destroy(floor3);
            Destroy(floor4);
            Destroy(floor5);
            Destroy(floor6);
            Destroy(GizmoX);
            Destroy(GizmoY);
            Destroy(GizmoZ);
            Destroy(X150_obj);
            fish = Instantiate(fish_obj_prefab);
            X150_obj = Instantiate(X150_prefab, new Vector3(0, tankScale_Z / 2f, 0), Quaternion.identity);
            for (int i = 0; i < cnt; i++)
            {
                Destroy(point_Obj_List[i]);
            }
            for (int i = 0; i < cnt; i++)
            {
                WayPoints.RemoveAt(WayPoints.Count - 1);
                line.RemoveAt(line.Count - 1);
                point_Text_List.RemoveAt(point_Text_List.Count - 1);
                point_Position.RemoveAt(point_Position.Count - 1);
                point_Obj_List.RemoveAt(point_Obj_List.Count - 1);
            }
            cnt = 0;
            warning_txt.text = ("수조가 생성되었습니다.");
            Invoke("Reset_Warning_txt", 2f);

            //수조 벽면 생성
            floor1 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor1.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Y / 10f);
            floor1.transform.position = new Vector3(0, -tankScale_Z / 2f, 0);

            floor2 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor2.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Y / 10f);
            floor2.transform.position = new Vector3(0, tankScale_Z / 2f, 0);
            floor2.transform.rotation = Quaternion.Euler(new Vector3(0, 0, 180f));

            floor3 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor3.transform.localScale = new Vector3(tankScale_Z / 10f, 1, tankScale_Y / 10f);
            floor3.transform.position = new Vector3(tankScale_X / 2f, 0, 0);
            floor3.transform.rotation = Quaternion.Euler(new Vector3(0, 0, 90f));

            floor4 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor4.transform.localScale = new Vector3(tankScale_Z / 10f, 1, tankScale_Y / 10f);
            floor4.transform.position = new Vector3(-tankScale_X / 2f, 0, 0);
            floor4.transform.rotation = Quaternion.Euler(new Vector3(0, 0, -90f));

            floor5 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor5.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Z / 10f);
            floor5.transform.position = new Vector3(0, 0, tankScale_Y / 2f);
            floor5.transform.rotation = Quaternion.Euler(new Vector3(-90f, 0, 0));

            floor6 = Instantiate(Floor_prefab, Vector3.zero, Quaternion.identity);
            floor6.transform.localScale = new Vector3(tankScale_X / 10f, 1, tankScale_Z / 10f);
            floor6.transform.position = new Vector3(0, 0, -tankScale_Y / 2f);
            floor6.transform.rotation = Quaternion.Euler(new Vector3(90f, 0, 0));

            //X,Y,Z 라인 생성
            GizmoX = Instantiate(Xline_prefab, Vector3.zero, Quaternion.identity);
            GizmoX.transform.localScale = new Vector3(0.05f, tankScale_X / 2f, 0.05f);
            GizmoX.transform.position = new Vector3(0, -tankScale_Z / 2f, -tankScale_Y / 2f);
            GizmoX.transform.rotation = Quaternion.Euler(new Vector3(90f, 0, 90f));

            GizmoY = Instantiate(Yline_prefab, Vector3.zero, Quaternion.identity);
            GizmoY.transform.localScale = new Vector3(0.05f, tankScale_Y / 2f, 0.05f);
            GizmoY.transform.position = new Vector3(-tankScale_X / 2f, -tankScale_Z / 2f, 0);
            GizmoY.transform.rotation = Quaternion.Euler(new Vector3(0, 90f, 90f));

            GizmoZ = Instantiate(Zline_prefab, Vector3.zero, Quaternion.identity);
            GizmoZ.transform.localScale = new Vector3(0.05f, tankScale_Z / 2f, 0.05f);
            GizmoZ.transform.position = new Vector3(-tankScale_X / 2f, 0, -tankScale_Y / 2f);

            obj = Instantiate(CirclularTank_prefab, Vector3.zero, Quaternion.identity);
            obj.transform.localScale = new Vector3(tankScale_X, tankScale_Z, tankScale_Y);

            GameObject.Find("Camera Arm").GetComponent<Transform>().rotation = Quaternion.Euler(0, 0, 0);

            Origin = new Vector3(0, tankScale_Z / 2f, 0);

        }
        else if (index == 0)
        {
            warning_txt.text = ("수조 모양을 선택해주세요.");
            Invoke("Reset_Warning_txt", 2f);
        }
        else
        {
            warning_txt.text = ("입력값이 잘못되었습니다.");
            Invoke("Reset_Warning_txt", 2f);
        }
        //이미 생성되어 있을 경우 씬이 전환되어도 사라지지 않도록 유지
        if (obj != null)
        {
            isTank = true;
            DontDestroyOnLoad(obj);
            DontDestroyOnLoad(GizmoX);
            DontDestroyOnLoad(GizmoY);
            DontDestroyOnLoad(GizmoZ);
            DontDestroyOnLoad(floor1);
            DontDestroyOnLoad(floor2);
            DontDestroyOnLoad(floor3);
            DontDestroyOnLoad(floor4);
            DontDestroyOnLoad(floor5);
            DontDestroyOnLoad(floor6);
            DontDestroyOnLoad(X150_obj);
            DontDestroyOnLoad(fish);
        }

        GetDistance();
    }
    //좌표 저장 클릭
    private void Click_Setposition()
    {
        if (GameObject.Find("Set Postion Button") != null)
        {
            GameObject.Find("Set Postion Button").GetComponent<Button>().onClick.RemoveListener(setposition);
            GameObject.Find("Set Postion Button").GetComponent<Button>().onClick.AddListener(setposition);
        }
    }
    //트랙 저장 클릭
    private void Click_Save_Track()
    {
        if (GameObject.Find("Set Track Button") != null)
        {
            GameObject.Find("Set Track Button").GetComponent<Button>().onClick.RemoveListener(savetrack);
            GameObject.Find("Set Track Button").GetComponent<Button>().onClick.AddListener(savetrack);
        }
    }
    //좌표 초기화 클릭
    private void Click_Remove_Setpostion()
    {
        if (GameObject.Find("Remove Set Postion Button") != null)
        {
            GameObject.Find("Remove Set Postion Button").GetComponent<Button>().onClick.RemoveListener(remove_Setpostion);
            GameObject.Find("Remove Set Postion Button").GetComponent<Button>().onClick.AddListener(remove_Setpostion);
        }
    }
    //X150 위치 변경 팝업 클릭
    public void Click_X150_PopUp()
    {
        if (GameObject.Find("X150 Relocate Button") != null && isTank)
        {
            GameObject.Find("X150 Relocate Button").GetComponent<Button>().onClick.RemoveListener(Create_X150_PopUp);
            GameObject.Find("X150 Relocate Button").GetComponent<Button>().onClick.AddListener(Create_X150_PopUp);
        }
    }
    //X150 위치 변경 팝업 제거 클릭
    public void Click_Remove_X150_PopUp()
    {
        if (GameObject.Find("Pop Up Delete Button") != null)
        {
            GameObject.Find("Pop Up Delete Button").GetComponent<Button>().onClick.RemoveListener(Remove_X150_PopUp);
            GameObject.Find("Pop Up Delete Button").GetComponent<Button>().onClick.AddListener(Remove_X150_PopUp);
        }
    }
    //X150 위치 변경 팝업 생성
    public void Create_X150_PopUp()
    {
        GameObject.Find("Main Menu").transform.Find("X150 Relocate Pop-Up").gameObject.SetActive(true);
        if (GameObject.Find("Relocation Range") != null)
            relocate_Box = GameObject.Find("Relocation Range");
        if (GameObject.Find("Relocation Frame") != null)
            relocate_Frame = GameObject.Find("Relocation Frame");
        X150_Relocate_Set_Boxsize();
    }
    //X150 위치 변경 팝업 제거
    public void Remove_X150_PopUp()
    {
        Debug.Log("제거");
        GameObject.Find("Main Menu").transform.Find("X150 Relocate Pop-Up").gameObject.SetActive(false);
        input_x150_X_Relo.text = "";
        input_x150_Y_Relo.text = "";
    }

    //좌표 초기화
    public void remove_Setpostion()
    {
        for (int i = 0; i < cnt; i++)
        {
            Destroy(point_Obj_List[i]);
        }
        for (int i = 0; i < cnt; i++)
        {
            WayPoints.RemoveAt(WayPoints.Count - 1);
            line.RemoveAt(line.Count - 1);
            point_Text_List.RemoveAt(point_Text_List.Count - 1);
            point_Position.RemoveAt(point_Position.Count - 1);
            point_Obj_List.RemoveAt(point_Obj_List.Count - 1);
        }
        cnt = 0;
    }
    //좌표 저장
    public void setposition()
    {
        if (isTank && Position_Check_In_Tank())
        {
            //좌표 리스트 생성
            WayPoints.Add(new List<float>());
            WayPoints[cnt].Add(float.Parse(inputDirection_X.text));
            WayPoints[cnt].Add(float.Parse(inputDirection_Y.text));
            WayPoints[cnt].Add(float.Parse(inputDirection_Z.text));

            //X150과 물고기와의 거리 Vector값
            dist_vec = new Vector3(WayPoints[cnt][0], -WayPoints[cnt][2], WayPoints[cnt][1]);

            //설정 좌표에 좌표 오브젝트 생성
            GameObject point = Instantiate(point_Obj_prefab, dist_vec + Origin, Quaternion.identity);

            //좌표 오브젝트 vector값 리스트 추가
            point_Position.Add(point.transform.position);
            //좌표 오브젝트 리스트 추가
            point_Obj_List.Add(point);
            //좌표 번호 표시 텍스트 리스트 추가
            point_Text_List.Add(point.GetComponentInChildren<TextMesh>());
            //라인 표시 리스트 추가
            line.Add(point.GetComponent<LineRenderer>());

            numbering_Point_Text();

            cnt++;
            for (int i = 0; i < cnt; i++)
            {
                DontDestroyOnLoad(point_Obj_List[i]);
            }
            warning_txt.text = ("좌표가 생성되었습니다.");
            Invoke("Reset_Warning_txt", 2f);
        }
        else if (!isTank)
        {
            warning_txt.text = ("수조가 생성되지 않았습니다.");
            Invoke("Reset_Warning_txt", 2f);
        }
        else
        {
            warning_txt.text = ("입력값이 잘못되었습니다.");
            Invoke("Reset_Warning_txt", 2f);
        }
    }
    //좌표 번호 표시
    private void numbering_Point_Text()
    {
        for (int i = 0; i <= cnt; i++)
        {
            point_Text_List[i] = point_Obj_List[i].GetComponentInChildren<TextMesh>();
            point_Text_List[i].text = "P-" + (i + 1).ToString();
            if (i > 0)
                Draw_Track(i);
        }
    }
    //설정 경로 그리기
    private void Draw_Track(int i)
    {
        if (point_Obj_List[i - 1] != null)
        {
            line[i - 1] = point_Obj_List[i - 1].GetComponent<LineRenderer>();
            line[i - 1].enabled = true;
            Track_Line_Setting(i);
        }
    }
    //설정 경로 점선 세팅
    private void Track_Line_Setting(int i)
    {
        //LineRenderer 컴포넌트를 사용하여 현재 좌표와 이전 좌표의 선을 그려쥼
        line[i - 1].startWidth = .05f;
        line[i - 1].endWidth = .05f;

        before_Point = point_Position[i - 1];
        cur_Point = point_Position[i];

        line[i - 1].SetPosition(0, before_Point);
        line[i - 1].SetPosition(1, cur_Point);
        line[i - 1].material.mainTextureScale = new Vector2(15, 1);
    }
    //좌표 파일로 저장
    public void savetrack()
    {
        FileStream f = new FileStream(inputFileName.text + ".txt", FileMode.OpenOrCreate, FileAccess.Write);
        StreamWriter writer = new StreamWriter(f, System.Text.Encoding.UTF8);

        for (int i = 0; i < cnt; i++)
        {
            writer.WriteLine("Point-" + (i + 1) + ", X : " + (-point_Position[i].x).ToString() + ", Y : " + (-point_Position[i].z).ToString() + ", Z : " + ((-point_Position[i].y) + tankScale_Y / 2f).ToString());
            Destroy(point_Obj_List[i]);
        }
        for (int i = 0; i < cnt; i++)
        {
            WayPoints.RemoveAt(WayPoints.Count - 1);
            line.RemoveAt(line.Count - 1);
            point_Text_List.RemoveAt(point_Text_List.Count - 1);
            point_Position.RemoveAt(point_Position.Count - 1);
            point_Obj_List.RemoveAt(point_Obj_List.Count - 1);
        }
        writer.Flush();
        writer.Close();
        cnt = 0;
        warning_txt.text = ("경로가 저장되었습니다.");
        Invoke("Reset_Warning_txt", 3f);
    }
    //좌표가 수조 내부에 있는지 확인
    private bool Position_Check_In_Tank()
    {
        //수조 크기 X, Y의 절반 값
        float half_tank_X = temp_tankScale_X / 2;
        float half_tank_Y = temp_tankScale_Z / 2;
        //원점, 즉 X150의 좌표를 기준으로 입력한 좌표값이 수조 크기를 벗어났는지 확인
        if ((half_tank_X - Origin.x >= float.Parse(inputDirection_X.text)) && (-(half_tank_X + Origin.x) <= float.Parse(inputDirection_X.text)) &&
            (half_tank_Y - Origin.z >= float.Parse(inputDirection_Y.text)) && (-(half_tank_Y + Origin.z) <= float.Parse(inputDirection_Y.text)) &&
            float.Parse(inputDirection_Z.text) >= 0 && float.Parse(inputDirection_Z.text) <= temp_tankScale_Y)
        {
            return true;
        }
        else
        {
            warning_txt.text = ("좌표를 생성할 수 없습니다.");
            Invoke("Reset_Warning_txt", 2f);
            return false;
        }
    }
    //알림 문구 초기화
    private void Reset_Warning_txt()
    {
        warning_txt.text = ("");
    }
    //Track Run 파일 초기화 
    public void Reset_Track_Run()
    {
        for (int i = 0; i < lineCnt; i++)
        {
            Destroy(file_Point_Obj_List[i]);
            Destroy(traking_Obj_List[i]);
        }
        for (int i = 0; i < lineCnt; i++)
        {
            traking_Line.RemoveAt(traking_Line.Count - 1);
            traking_Text_List.RemoveAt(traking_Text_List.Count - 1);
            traking_Obj_Vector.RemoveAt(traking_Obj_Vector.Count - 1);
            traking_Obj_List.RemoveAt(traking_Obj_List.Count - 1);
        }
        select_file_item.Clear();
        file_Point_Obj_List.Clear();
        lineCnt = 0;
    }
    //Track Run 파일 불러오기
    public void Get_File_Name()
    {
        DirectoryInfo di = new DirectoryInfo(@"C:");
        FileInfo[] files = di.GetFiles("*.csv");
        //string str = "";

        string value = "";

        int line_cnt = 0;

        FileNamedropdown.ClearOptions();
        File_Name.Clear();
        File_Name.Add("경로");
        foreach (FileInfo file in files)
        {
            File_Name.Add(file.Name.ToString());
        }
        FileNamedropdown.AddOptions(File_Name);

        FileNamedropdown.onValueChanged.RemoveListener(Select_File_Name);
        FileNamedropdown.onValueChanged.AddListener(Select_File_Name);

        if (GameObject.Find("File Name Label") != null)
        {
            Filename_Label = GameObject.Find("File Name Label").GetComponent<Text>();
            Filename_Label.text = FileNamedropdown.options[file_index].text;
        }

        string select_filename = Filename_Label.text;
        int index = FileNamedropdown.value;
        for (index = 0; index < files.Length; index++)
        {
            if (select_filename.Equals(FileNamedropdown.options[index + 1].text))
            {
                if (file_Change)
                {
                    remove_Setpostion();
                    Reset_Track_Run();

                    file_Change = false;
                    int idx_num = 0;
                    //Get_Text_Read(files[index].ToString());
                    StreamReader reader = new StreamReader(select_filename);
                    while (true)
                    {
                        value = reader.ReadLine();
                        if (value == null)
                        {
                            break;
                        }
                        string[] arr = value.Split(',');

                        select_file_item.Add(new List<float>());
                        for (int j = 0; j < 9; j++)
                        {
                            select_file_item[idx_num].Add(float.Parse(arr[j]));
                        }
                        idx_num++;
                        lineCnt++;
                    }
                    for (int k = 0; k < lineCnt; k++)
                    {
                        GameObject obj = GameObject.Instantiate(position_List_Prefab) as GameObject;
                        obj.transform.parent = GameObject.Find("Item Correct").transform;
                        obj.transform.localScale = Vector3.one;
                        file_Point_Obj_List.Add(obj);
                        file_Point_Obj_List[k].GetComponent<Button>().GetComponentInChildren<Text>().text = "NO." + (k + 1);
                        file_Point_Obj_List[k].transform.GetChild(1).gameObject.GetComponentInChildren<Text>().text = select_file_item[k][1].ToString("F2");
                        file_Point_Obj_List[k].transform.GetChild(2).gameObject.GetComponentInChildren<Text>().text = select_file_item[k][2].ToString("F2");
                        file_Point_Obj_List[k].transform.GetChild(3).gameObject.GetComponentInChildren<Text>().text = select_file_item[k][3].ToString("F2");

                        traking_point_vector = new Vector3(select_file_item[k][1], -select_file_item[k][3] + temp_tankScale_Y / 2f, select_file_item[k][2]);
                        Debug.Log(traking_point_vector);
                        X110 = Instantiate(X110_prefab, traking_point_vector, Quaternion.identity);
                        traking_Obj_List.Add(X110);
                        traking_Obj_Vector.Add(X110.transform.position);
                        traking_Text_List.Add(X110.GetComponentInChildren<TextMesh>());
                        traking_Line.Add(X110.GetComponent<LineRenderer>());
                        DontDestroyOnLoad(traking_Obj_List[k]);

                    }
                    Numbering_Traking_Text();
                    reader.Close();
                }

            }
        }

    }
    //좌표 번호 표시하는 함수
    public void Numbering_Traking_Text()
    {
        for (int i = 0; i < lineCnt; i++)
        {
            traking_Text_List[i] = traking_Obj_List[i].GetComponentInChildren<TextMesh>();
            traking_Text_List[i].text = (i + 1).ToString();
            traking_Obj_List[i].GetComponent<MeshRenderer>().material.color = Color.red;
            if (i != 0)
            {
                traking_Obj_List[i - 1].GetComponent<MeshRenderer>().material.color = Color.black;
            }

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
            Tracking_Line_Setting(i);
        }
    }
    //설정 경로 점선 세팅
    private void Tracking_Line_Setting(int i)
    {
        traking_Line[i - 1].startWidth = .05f;
        traking_Line[i - 1].endWidth = .05f;

        before_Point = traking_Obj_Vector[i - 1];
        cur_Point = traking_Obj_Vector[i];

        traking_Line[i - 1].SetPosition(0, before_Point);
        traking_Line[i - 1].SetPosition(1, cur_Point);
        traking_Line[i - 1].material.mainTextureScale = new Vector2(15, 1);
    }
    public void Select_File_Name(int value)
    {
        file_index = FileNamedropdown.value;
        file_Change = true;
    }
    //해당 파일이 존재할 경우 파일 읽기
    public void Get_Text_Read(string filePath)
    {
        FileInfo fileinfo = new FileInfo(filePath);
        string value = "";

        if (fileinfo.Exists)
        {
            StreamReader reader = new StreamReader(filePath);
            value = reader.ReadToEnd();
            reader.Close();
        }
        else
            value = "파일이 없습니다.";

    }
    //X150 위치 재배치
    public void X150_Relocate()
    {
        if (Check_X150_InTank())
        {
            Origin = new Vector3(x150_Relo_X, tankScale_Z / 2, x150_Relo_Y);
            Debug.Log(Origin);
            X150_clone.transform.position = Origin;
            X150_icon.GetComponent<RectTransform>().anchoredPosition = new Vector2(x150_Relo_X * (relocate_Box_Size.x / tankScale_X), x150_Relo_Y * (relocate_Box_Size.y / tankScale_Y));
            for (int i = 0; i < cnt; i++)
            {
                Destroy(point_Obj_List[i]);
            }
            for (int i = 0; i < cnt; i++)
            {
                WayPoints.RemoveAt(WayPoints.Count - 1);
                line.RemoveAt(line.Count - 1);
                point_Text_List.RemoveAt(point_Text_List.Count - 1);
                point_Position.RemoveAt(point_Position.Count - 1);
                point_Obj_List.RemoveAt(point_Obj_List.Count - 1);
            }
            cnt = 0;
        }
        else
        {
            warning_txt.text = ("범위를 벗어났습니다.");
            Invoke("Reset_Warning_txt", 3f);
        }
    }
    //X150 위치 초기화
    public void X150_Locate_Default()
    {
        Origin = new Vector3(0, tankScale_Z / 2f, 0);
        X150_clone.transform.position = Origin;
        input_x150_X_Relo.text = "";
        input_x150_Y_Relo.text = "";
        X150_icon.GetComponent<RectTransform>().anchoredPosition = new Vector2(0f, 0f);
        for (int i = 0; i < cnt; i++)
        {
            Destroy(point_Obj_List[i]);
        }
        for (int i = 0; i < cnt; i++)
        {
            WayPoints.RemoveAt(WayPoints.Count - 1);
            line.RemoveAt(line.Count - 1);
            point_Text_List.RemoveAt(point_Text_List.Count - 1);
            point_Position.RemoveAt(point_Position.Count - 1);
            point_Obj_List.RemoveAt(point_Obj_List.Count - 1);
        }
        cnt = 0;
    }
    //X150 재배치 시각화
    public void X150_Relocate_Set_Boxsize()
    {
        if (obj.tag == "RectangularTank")
        {
            relocate_Box.GetComponent<Image>().sprite = rectangle_Image;
            frame_Size_X = (relocate_Frame.GetComponent<RectTransform>().sizeDelta.x) * 0.8f;
            frame_Size_Y = (relocate_Frame.GetComponent<RectTransform>().sizeDelta.y) * 0.8f;
            if (tankScale_X > tankScale_Y)
            {
                relocate_Box_Size = new Vector2(frame_Size_X, tankScale_Y * (frame_Size_X / tankScale_X));
                relocate_Box.GetComponent<RectTransform>().sizeDelta = relocate_Box_Size;
            }
            else if (tankScale_Y > tankScale_X)
            {
                relocate_Box_Size = new Vector2(tankScale_X * (frame_Size_X / tankScale_Y), frame_Size_X);
                relocate_Box.GetComponent<RectTransform>().sizeDelta = relocate_Box_Size;
            }
            else
            {
                relocate_Box_Size = new Vector2(frame_Size_X, frame_Size_Y);
                relocate_Box.GetComponent<RectTransform>().sizeDelta = relocate_Box_Size;
            }
        }
        else if (obj.tag == "CircularTank")
        {
            relocate_Box.GetComponent<Image>().sprite = circle_Image;
            frame_Size_X = (relocate_Frame.GetComponent<RectTransform>().sizeDelta.x) * 0.8f;
            frame_Size_Y = (relocate_Frame.GetComponent<RectTransform>().sizeDelta.y) * 0.8f;
            if (tankScale_X > tankScale_Y)
            {
                relocate_Box_Size = new Vector2(frame_Size_X, tankScale_Y * (frame_Size_X / tankScale_X));
                relocate_Box.GetComponent<RectTransform>().sizeDelta = relocate_Box_Size;
            }
            else if (tankScale_Y > tankScale_X)
            {
                relocate_Box_Size = new Vector2(tankScale_X * (frame_Size_X / tankScale_Y), frame_Size_X);
                relocate_Box.GetComponent<RectTransform>().sizeDelta = relocate_Box_Size;
            }
            else
            {
                relocate_Box_Size = new Vector2(frame_Size_X, frame_Size_Y);
                relocate_Box.GetComponent<RectTransform>().sizeDelta = relocate_Box_Size;
            }
        }
    }
    //X150 재배치 시 수조 내부에 있는지 확인
    public bool Check_X150_InTank()
    {
        if (x150_Relo_X <= tankScale_X && x150_Relo_Y <= tankScale_Y)
            return true;
        else
            return false;
    }
    //Track Run 에서 run 버튼 클릭
    public void Run_Track()
    {

    }
}
