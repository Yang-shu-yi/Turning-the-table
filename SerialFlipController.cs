using UnityEngine;
using System.IO.Ports;
using System.Text;
using System.Collections;

public class SerialFlipController : MonoBehaviour
{
    [Header("Serial")]
    public string portName = "COM3";  // Windows: COMx ; macOS: /dev/cu.usbmodemXXXX
    public int baudRate = 9600;

    [Header("Flip Target")]
    public Transform cube;
    public float flipDuration = 0.45f;  // 過度動畫時間（秒）

    private SerialPort sp;
    private StringBuilder inbox = new StringBuilder();
    private bool isFlipping = false;

    void Start()
    {
        try {
            sp = new SerialPort(portName, baudRate);
            sp.ReadTimeout = 1;
            sp.NewLine = "\n";
            sp.Open();
            Debug.Log("Serial opened: " + portName);
        } catch (System.Exception e) {
            Debug.LogError("Open Serial failed: " + e.Message);
        }
    }

    void Update()
    {
        if (sp != null && sp.IsOpen) {
            try {
                string data = sp.ReadExisting();
                if (!string.IsNullOrEmpty(data))
                {
                    inbox.Append(data);
                    int nl;
                    while ((nl = inbox.ToString().IndexOf('\n')) >= 0)
                    {
                        string line = inbox.ToString(0, nl).Trim();
                        inbox.Remove(0, nl + 1);
                        ParseLine(line);
                    }
                }
            } catch { /* ignore timeouts */ }
        }
    }

    void ParseLine(string line)
    {
        // 期望訊息: TRIGGER:led=1
        if (line.StartsWith("TRIGGER:"))
        {
            // 只要收到觸發就翻一次
            TriggerFlip();
        }
        // 你也可以依需求使用 TRIAL:START 或 EVENT:LED_OFF 做 UI 提示
    }

    void TriggerFlip()
    {
        if (cube == null || isFlipping) return;
        StartCoroutine(FlipXCoroutine());
    }

    IEnumerator FlipXCoroutine()
    {
        isFlipping = true;
        Quaternion start = cube.rotation;
        Quaternion target = start * Quaternion.Euler(180f, 0f, 0f);

        float t = 0f;
        while (t < 1f)
        {
            t += Time.deltaTime / flipDuration;
            cube.rotation = Quaternion.Slerp(start, target, Mathf.SmoothStep(0f, 1f, t));
            yield return null;
        }
        cube.rotation = target;
        isFlipping = false;
    }

    void OnDestroy()
    {
        if (sp != null && sp.IsOpen) sp.Close();
    }
}
