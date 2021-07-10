#include<stdio.h>
#include<Windows.h>
#include<math.h>
#include<tchar.h>
#include<time.h>

//计算BPM要用到的值
#define BPM 60000
//音符不可能达到的位置（初始化用）
#define INF -100

typedef struct NOTE {
    int num;    //音符按键的位置
    int t;        //音符的时长
    NOTE *next;    //下一个结点
} Note;

//键位设置
char f[] = {'Z', '1', 'X', '2', 'C', 'V', '3', 'B', '4', 'N', '5', 'M', 'A', '6', 'S', '7', 'D', 'F', '8', 'G', '9',
            'H', '0', 'J', 'Q', 'K', 'W', 'L', 'E', 'R', 'O', 'T', 'P', 'Y', static_cast<char>(188), 'U', 'I'};
int bpm = 0;    //曲速
int noteSet = 0;//获取谱号后音符的位置
Note *tail;        //尾结点（尾插法建立链表）
int out = 0;    //超出ff14音域多少
int count = 0;

//判断最前的窗口是否为ff14游戏窗口
int isFF14Window() {
    HWND hwnd = GetForegroundWindow();
    char gameWindowText[] = "最终幻想XIV";
    wchar_t windowText[256];
    GetWindowText(hwnd, reinterpret_cast<LPSTR>(windowText), 256);
    if (!_tcscmp(reinterpret_cast<const char *>(windowText), reinterpret_cast<const char *>(gameWindowText)))
        return 1;
    return 0;
}

//修改谱号
void SetClef(FILE *fp) {
    char b, c[3] = {0};
    int i;
    for (i = 0, b = fgetc(fp); b != '\n'; i++, b = fgetc(fp)) {
        c[i] = b;
    }
    noteSet = 12;    //C调
    switch (c[0]) {
        case 'C':
            break;
        case 'D':
            noteSet += 2;
            break;
        case 'E':
            noteSet += 4;
            break;
        case 'F':
            noteSet += 5;
            break;
        case 'G':
            noteSet += 7;
            break;
        case 'A':
            noteSet += 9;
            break;
        case 'B':
            noteSet += 11;
            break;
    }
    switch (c[1]) {
        case '#':
            noteSet += 1;
            break;
        case 'b':
            noteSet -= 1;
            break;
    }
}

//修改BPM
int SetBPM(FILE *fp) {
    int i, j;
    char b;
    char fbpm[3];
    for (i = 0, b = fgetc(fp); b != '\n'; i++, b = fgetc(fp))    //从文件里面获取fbpm
        fbpm[i] = b - 48;
    int bpm = 0;                    //初始化bpm
    for (j = 0; j < i; j++)
        bpm += pow(10, i - j - 1) * fbpm[j];        //从fbpm获取bpm
    bpm = BPM / bpm;                    //计算每一拍的时间，单位ms
    return bpm;
}

//初始化头结点
Note *InitNote(Note *&p) {
    p = (Note *) malloc(sizeof(Note));
    p->next = NULL;
    return p;
}

//尾插法插入结点
void InsertNote(int t, int num) {
    Note *p;
    InitNote(p);
    p->num = num;
    int temp = num - 36;
    if (temp > out) {
        printf("%d 处超出了 %d\n", count, temp);
        out = temp;
    }
    count += 1;
    p->t = t;
    tail->next = p;
    tail = tail->next;
}

//调整音高
void SetPitch(char b, int *pitch) {
    *pitch = 0;
    switch (b) {
        case '0':
            *pitch = INF;
        case '1':
            break;
        case '2':
            *pitch += 2;
            break;
        case '3':
            *pitch += 4;
            break;
        case '4':
            *pitch += 5;
            break;
        case '5':
            *pitch += 7;
            break;
        case '6':
            *pitch += 9;
            break;
        case '7':
            *pitch += 11;
            break;    //调整音高
    }
}

//建立链表
void CreateList(FILE *fp, Note *&list) {
    out = 0;
    count = 0;
    InitNote(list);
    tail = list;
    printf("建立链表中...\n");
    SetClef(fp);
    bpm = SetBPM(fp);
    char b, a;
    for (b = fgetc(fp); b != EOF; b = fgetc(fp))    //获取音符
    {
        if (b == '\n') {    //改变谱号和BPM
            SetClef(fp);
            bpm = SetBPM(fp);
            continue;
        }
        int t = bpm, delay = 1, pitch = 0, multiNotesCount = 0;
        SetPitch(b, &pitch);
        for (a = fgetc(fp); a != ' ' && a != EOF; a = fgetc(fp)) {
            switch (a) {
                case '`':
                    pitch += 12;
                    break;        //上升1个八度
                case '.':
                    pitch -= 12;
                    break;        //下降一个八度
                case '_':
                    t /= 2;
                    delay += 1;
                    break;    //时值减半（初始为四分音符的时值）
                case '-':
                    t += bpm;
                    break;        //时值加一个四分音符的时值
                case '#':
                    pitch += 1;
                    break;        //升号
                case 'b':
                    pitch -= 1;
                    break;        //降号
                case ':':
                    t += bpm / (2 * delay);
                    break;    //附点音符
                default:
                    InsertNote(20, pitch + noteSet);
                    SetPitch(a, &pitch);
                    multiNotesCount += 1;
            }
        }
        t = t - multiNotesCount * 20 > 20 ? t - multiNotesCount : t;
        InsertNote(t, pitch + noteSet);
    }
    printf("建立链表完成！\n");
    fclose(fp);
}

// "醒着睡觉"
void wakeSleep(long milliSec) {
    struct timeb start;
    ftime(&start);
    struct timeb end;
    while (1) {
        ftime(&end);
        if (end.millitm + end.time * 1000 - start.time * 1000 - start.millitm > milliSec) {
            return;
        }
    }
}

//把链表转化成按键信号
void ClickList(Note *list) {
    Sleep(100);
    printf("开始弹奏\n");
    Note *p = list->next;
    while (p != NULL) {
        if (!isFF14Window()) {
            printf("弹奏停止\n");
            return;
        }
        if (p->num < 0) {
            wakeSleep(p->t);
            p = p->next;
            continue;
        }
        keybd_event(f[p->num], 0, 0, 0);
        // 没有封装成函数，减少频繁开函数栈帧带来的性能损失。想要函数在上面封装好了 (实测封装成函数要多1ms)
        struct timeb start;
        struct timeb end;
        ftime(&start);
        while (1) {
            ftime(&end);
            if (end.millitm + end.time * 1000 - start.time * 1000 - start.millitm > p->t - (p->t) / 4) {
                break;
            }
        }
//        Sleep(p->t - (p->t) / 4);
        keybd_event(f[p->num], 0, 2, 0);
        ftime(&start);
        while (1) {
            ftime(&end);
            if (end.millitm + end.time * 1000 - start.time * 1000 - start.millitm > (p->t) / 4) {
                break;
            }
        }
//        Sleep((p->t) / 4);
        p = p->next;
    }
    printf("弹奏结束\n");
}

//通过降低整个曲子的调强制把超出部分限制在ff14音域内
void LimitNote(Note *&list) {
    printf("超出了 %d\n", out);
    Note *p = list;
    while (p != NULL) {
        p->num -= out;
        if (p->num < 0) {
            p->num = INF;
        }
        p = p->next;
    }
}

//销毁链表
void DestroyList(Note *&list) {
    Note *p = list->next;
    while (p != NULL) {
        free(list);
        list = p;
        p = p->next;
    }
    free(list);
    list = NULL;
}

int main() {
    system("title ff14演奏ver1.5");
    system("mode con cols=70 lines=40");
    char a[100];
    FILE *fp;
    while (true) {
        system("cls");
        for (int i = 0; i < 100; i++)
            a[i] = '\0';
        printf("\n play text file：");
        gets(a);
        fp = fopen(a, "r");    //打开文本文件
        if (fp == NULL)
            printf("\n cannot find the file: %s\n", a);
        else {
            printf("----切换到最终幻想14游戏中开始弹奏----");
            while (!isFF14Window())
                Sleep(100);    //每0.1秒检测ff14窗口，检测到了就退出循环

            Note *list;
            CreateList(fp, list);
            LimitNote(list);
            ClickList(list);
            DestroyList(list);
        }
        system("pause");
    }
}