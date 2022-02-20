#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include<queue>

using namespace std;
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

#pragma pack(1) //���ö��䷽ʽ����1�ֽڶ���
//����������
struct BPB {
    char BS_OEMName[8];     //OEM�ַ�����mkfs.fat
    ushort BPB_BytsPerSEC;  //ÿ�����ֽ�����512
    byte BPB_SecPerClus;    //ÿ��ռ�õ���������1
    ushort BPB_RsvdSecCnt;  //bootռ�õ���������1
    byte BPB_NumFATs;       //FAT��ļ�¼����2
    ushort BPB_RootEntCnt;  //����Ŀ¼�ļ�����224
    ushort BPB_TotSec16;    //�߼�����������2880
    byte BPB_Media;         //ý����������240
    ushort BPB_FATSz16;     //ÿ��FATռ�õ���������9
    ushort BPB_SecPerTrk;   //ÿ���ŵ���������18
    ushort BPB_NumHeads;    //��ͷ����2
    uint BPB_Hiddsec;       //������������0
    uint BPB_TotSec32;      // ���BPB_TotSec16��0�����������¼��0
    byte BS_DrvNum;         // �ж�13���������ţ�0
    byte BS_Reserved1;      // δʹ�ã�1
    byte BS_BootSig;        // ��չ������־��41
    uint BS_VolID;          // �����к�
    byte BS_VolLab[11];     // ���
    byte BS_FileSysType[8]; // �ļ�ϵͳ���ͣ�FAT12
};

//��Ŀ¼��Ŀ¼��
struct RootEntry {
    char DIR_Name[11];  //�ļ���8�ֽڣ���չ��3�ֽ�
    byte DIR_Attr;      //�ļ�����
    byte reserve[10];   //����λ
    ushort DIR_WrtTime; //���һ��д��ʱ��
    ushort DIR_WrtDate; //���һ��д������
    ushort DIR_FstClus; //�ļ���ʼ�Ĵغ�
    uint DIR_FileSize;  //�ļ���С
};
#pragma pack()

vector<string> commandSplit(string &);

void ls(queue<string> &);

void lsl(queue<string>);

void lsOut(vector<pair<string, char> >);

void trim(string &);

vector<string> split(string, string);

extern "C" {
void sprint(const char *);
};

void solveInput();

RootEntry findCluster(string, int);

int calNextCluster(int);

void findSubDirectoryAndFile(queue<string> &, string path, RootEntry &);

string getName(RootEntry);

string getExtension(RootEntry);

void calNextLevel(RootEntry, bool, string);

void cat(string path);

void redPrint(const char *s) {
//    cout << s;
    sprint("\033[31m");
    sprint(s);
    sprint("\033[0m");
}

void whitePrint(const char *s) {
    sprint(s);
//    cout << s;
}


BPB bpb;
vector<RootEntry> rootEntries(224);
ifstream fin;
char *fat1;
vector<pair<string, char> > toPrint; //����ӡ�Ķ���
vector<RootEntry> nextLevel;    //���ļ��к��ļ���Ŀ¼��






void lsl(queue<string> directories) {
    while (!directories.empty()) {
        queue<string> q;
        string path = directories.front();
        directories.pop();
        if (path == "/") {
            //forѭ��������q��Ӧ�ñ����Ŀ¼�µ�������Ŀ¼��nextLevel��Ӧ�ñ����Ŀ¼������ָ����Ŀ¼�����ļ���Ŀ¼��
            for (int i = 0; i < rootEntries.size(); i++) {
                RootEntry current = rootEntries[i];
                findSubDirectoryAndFile(q, path, current);
            }

            //queue��������Ŀ¼
            int subDirectories = q.size();
            int subFiles = nextLevel.size() - subDirectories;
            //cout << path << " " << subDirectories << " " << subFiles << ":" << endl;
            string s = path + " " + to_string(subDirectories) + " " + to_string(subFiles) + ":" + "\n";
            whitePrint(s.c_str());

            for (int i = 0; i < nextLevel.size(); i++) {     //���ڸ�Ŀ¼�µ�ÿһ��
                calNextLevel(nextLevel[i], true, "/");
            }
        } else {
            vector<string> hierarchicalDirectories = split(path.substr(1, path.size() - 1), "/");
            int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //��ʼ�غ��Ǹ�Ŀ¼���״�
            RootEntry current;
            for (int i = 0; i < rootEntries.size(); i++) {        //���ҵ���һ��Ŀ¼����ת����������˵
                string name = getName(rootEntries[i]);
                if (name == hierarchicalDirectories[0]) {
                    startCluster = rootEntries[i].DIR_FstClus;
                    current = rootEntries[i];
                    break;
                } else {
                    startCluster = -1;
                }
            }

            if (startCluster == -1) {
                throw "Wrong Path!\n";
            }

            for (int i = 1; i < hierarchicalDirectories.size(); i++) {
                current = findCluster(hierarchicalDirectories[i], startCluster);    //����һ��Ŀ¼��ʼ�Ĵ���Ѱ�ҵ�ǰĿ¼��ʼ�Ĵ�
                startCluster = current.DIR_FstClus;
                if (startCluster == -1 || startCluster >= 0xFF8) { //��ǰĿ¼������
                    throw "Wrong Path!\n";
                }
            }
            //current���ڱ�����ָ��ָ��Ŀ¼��Ŀ¼��
            calNextLevel(current, false, path);     //��������ĵ�һ��

            //����currentCluster������ݹ��ӡ��Ŀ¼����ʼ��
            int currentCluster = startCluster;   //��һ��
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
            while (true) {
                //ÿ��Ѱ��һ����
                for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
                    //���ζ�ȡָ��Ŀ¼�µ�ÿһ��Ŀ¼��
                    fin.read((char *) &current, sizeof(current));
                    findSubDirectoryAndFile(q, path, current);
                }
                currentCluster = calNextCluster(currentCluster);
                if (currentCluster >= 0xFF8) {
                    break;
                }
            }
            for (int i = 0; i < nextLevel.size(); i++) {
                calNextLevel(nextLevel[i], true, path);
            }
        }
        //cout << endl;
        whitePrint("\n");
        nextLevel.clear();      //��ֹ�Եݹ��������
        lsl(q);
        toPrint.clear();        //��ֹ�����������������
    }
}

void calNextLevel(RootEntry temp, bool sub, string path) {    //path��������·��
    string directoryName = getName(temp);
    getName(temp);
    if (directoryName == "." || directoryName == "..") {
        //cout << directoryName << endl;
        string s = directoryName + "\n";
        redPrint(s.c_str());
        return;
    }

    int subDirectories = 0;
    int subFiles = 0;

    if (temp.DIR_Attr == 0x20) {    //��ǰΪ�ļ�
        string name = getName(temp);
        string extension = getExtension(temp);
        //cout << name << "." << extension << "  " << temp.DIR_FileSize << endl;
        string s = name + "." + extension + "  " + to_string(temp.DIR_FileSize) + "\n";
        whitePrint(s.c_str());
    } else {  //��ǰΪ�ļ���
        int currentCluster = temp.DIR_FstClus;
        fin.seekg((bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                  bpb.BPB_RootEntCnt * 32, ios::beg);
        while (true) {
            //ÿ��Ѱ��һ����
            for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
                RootEntry current;
                fin.read((char *) &current, sizeof(current));
                if (current.DIR_Name[0] != '\xe5') {
                    if (current.DIR_Attr == 0x20) {   //�ļ�
                        subFiles++;
                    } else if (current.DIR_Attr == 0x10) {    //�ļ���
                        string name = getName(current);
                        if (name != "." && name != "..") {
                            subDirectories++;
                        }
                    }
                }
            }

            currentCluster = calNextCluster(currentCluster);
            if (currentCluster >= 0xFF8) {
                if (sub) {   //��ǰ�ļ�������Ϊ���ļ��н���ͳ�Ƶ�
                    //cout << directoryName << " " << subDirectories << " " << subFiles << endl;
                    string s1 = directoryName + " ";
                    string s2 = to_string(subDirectories) + " " + to_string(subFiles) + "\n";
                    redPrint(s1.c_str());
                    whitePrint(s2.c_str());
                } else {      //��ǰ�ļ�������Ϊ���ļ���ͳ�Ƶ�
                    //cout << path << "/" << " " << subDirectories << " " << subFiles << ":" << endl;
                    string s = path + "/" + " " + to_string(subDirectories) + " " + to_string(subFiles) + ":" "\n";
                    whitePrint(s.c_str());
                }
                break;
            }
        }
    }
}


void ls(queue<string> &directories) {
    while (!directories.empty()) {      //������Ŀ¼��Ϊ��
        queue<string> q;
        string path = directories.front();  //ȡ������Ԫ�أ���������һ��Ŀ¼
        directories.pop();
        if (path == "/") {   //���ʸ�Ŀ¼����Ŀ¼
            //cout << "/" << ":" << endl;
            string s = "/:\n";
            whitePrint(s.c_str());
            for (int i = 0; i < rootEntries.size(); i++) {
                RootEntry &current = rootEntries[i];
                findSubDirectoryAndFile(q, path, current);
            }
        } else {
            //cout << path << "/" << ":" << endl;
            vector<string> hierarchicalDirectories = split(path.substr(1, path.size() - 1), "/");
            int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //��ʼ�غ��Ǹ�Ŀ¼���״�
            for (int i = 0; i < rootEntries.size(); i++) {        //���ҵ���һ��Ŀ¼����ת����������˵
                string name = "";
                name = getName(rootEntries[i]);
                if (name == hierarchicalDirectories[0]) {
                    startCluster = rootEntries[i].DIR_FstClus;
                    break;
                } else{
                    startCluster = -1;
                }
            }

            if (startCluster == -1) {
                throw "Wrong Path!\n";
            }


            for (int i = 1; i < hierarchicalDirectories.size(); i++) {
                startCluster = findCluster(hierarchicalDirectories[i],
                                           startCluster).DIR_FstClus;    //����һ��Ŀ¼��ʼ�Ĵ���Ѱ�ҵ�ǰĿ¼��ʼ�Ĵ�
                if (startCluster == -1 || startCluster >= 0xFF8) { //��ǰĿ¼������
                    throw "Wrong Path!\n";
                }
            }

            string s = path + "/:\n";
            whitePrint(s.c_str());

            //����currentCluster������ݹ��ӡ��Ŀ¼����ʼ��
            int currentCluster = startCluster;   //��һ��
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
            while (true) {
                //ÿ��Ѱ��һ����
                for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
                    RootEntry current;
                    fin.read((char *) &current, sizeof(current));
                    findSubDirectoryAndFile(q, path, current);
                }
                currentCluster = calNextCluster(currentCluster);
                if (currentCluster >= 0xFF8) {
                    break;
                }
            }
        }
        //�ڵݹ������һ���֮ǰһ��Ҫ�Ȱѵ�ǰ��ε��������
        lsOut(toPrint);
        toPrint.clear();
        ls(q);
    }
}


//path����һ��Ŀ¼
//cluster ��һ��Ŀ¼��Ӧ���������ı���Ŀ¼�Ĵأ����������ǿ��
RootEntry findCluster(string path, int startCluster) {
    RootEntry temp;
    int currentCluster = startCluster;   //��һ��
    fin.seekg((bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
              bpb.BPB_RootEntCnt * 32, ios::beg);
    while (true) {
        //ÿ��Ѱ��һ����
        for (int i = 0; i < bpb.BPB_BytsPerSEC / 32; i++) {
            fin.read((char *) &temp, sizeof(temp));
            string name = getName(temp);
            string extension = getExtension(temp);
            if (extension != "") {
                name += ".";
                name += extension;
            }

            if (name == path) {
                return temp;    //�ҵ�ָ��Ŀ¼�ˣ������Ӧ���������Ĵط���
            }
        }
        currentCluster = calNextCluster(currentCluster);
        if (currentCluster >= 0xFF8) {
            temp.DIR_FstClus = -1;
            return temp;    //�Ѿ����굱ǰĿ¼��������Ŀ¼�ˣ���Ȼû���ҵ���Ӧ·��������-1;
        } else {
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
        }
    }
}


int calNextCluster(int currentCluster) {
    int startByte = (currentCluster / 2) * 3;
    byte firstByte = fat1[startByte];
    byte secondByte = fat1[startByte + 1];
    byte thirdByte = fat1[startByte + 2];
    int nextCluster;
    if (currentCluster % 2 == 0) { //��ǰΪż�������ݴأ���һ���ֽ���Ϊ�Ͱ�λ���ڶ����ֽ�ǰ4λ��Ϊ��4λ
        nextCluster = (firstByte) | ((secondByte & 0x0F) << 8);
    } else {
        nextCluster = (thirdByte << 4) | ((secondByte >> 4) & 0x0F);
    }
    return nextCluster;
}

void findSubDirectoryAndFile(queue<string> &q, string path, RootEntry &current) {
    if (current.DIR_Name[0] != '\xe5')  //��Ŀ¼���һ���ֽ��޸�Ϊe5����ʾ��Ŀ¼�Ѿ�ɾ��
    {
        pair<string, char> p;
        string name = "";
        string extension = "";
        name = getName(current);
        extension = getExtension(current);
        trim(name);
        trim(extension);
        if (current.DIR_Attr == 0x10) {   //��ǰΪ�ļ���
            p.first = name;
            p.second = 'r';
            if (name != "." && name != "..") {
                if (path == "/") {
                    q.push(path + name);        //������һ��Ŀ¼
                } else {
                    q.push(path + "/" + name);
                }
            }
            toPrint.push_back(p);
            nextLevel.push_back(current);
        } else if (current.DIR_Attr == 0x20) {  //��ǰΪ�ļ�
            p.first = name + "." + extension;
            p.second = 'w';
            toPrint.push_back(p);
            nextLevel.push_back(current);
        }
    }
}

void cat(string path) {
    vector<string> hierarchicalDirectories;
    if (path.find("/") == 0) {
        hierarchicalDirectories = split(path.substr(1, path.size() - 1), "/");
    } else {
        hierarchicalDirectories = split(path, "/");
    }
    if (hierarchicalDirectories.size() == 1) {    //�ļ��ڸ�Ŀ¼��
        int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //��ʼ�غ��Ǹ�Ŀ¼���״�
        RootEntry current;
        for (int i = 0; i < rootEntries.size(); i++) {        //���ҵ���һ��Ŀ¼����ת����������˵
            string name = getName(rootEntries[i]);
            name += ".";
            name += getExtension(rootEntries[i]);
            if (name == hierarchicalDirectories[0] && rootEntries[i].DIR_Attr == 0x20) {
                startCluster = rootEntries[i].DIR_FstClus;
                current = rootEntries[i];
                break;
            } else {
                startCluster = -1;
            }
        }
        if (startCluster == -1) {
            throw "File Not Exist!";
        }
        //����startCluster�б����˸��ļ�������������ʼ��
        int currentCluster = startCluster;
        char *content = new char[current.DIR_FileSize];
        int restSize = current.DIR_FileSize;
        char *nextPointer = content;
        while (true) {
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);
            if (restSize >= bpb.BPB_BytsPerSEC) {
                fin.read(nextPointer, bpb.BPB_BytsPerSEC);
                nextPointer += bpb.BPB_BytsPerSEC;
                restSize -= bpb.BPB_BytsPerSEC;
            } else {
                fin.read(nextPointer, restSize);
                break;
            }
            currentCluster = calNextCluster(currentCluster);
        }
        string s = "";
        for (int i = 0; i < current.DIR_FileSize; i++) {
            s += content[i];
        }
        s += "\n";
        whitePrint(s.c_str());
        //cout << endl;
        delete[] content;
    } else {
        int startCluster = bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16;  //��ʼ�غ��Ǹ�Ŀ¼���״�
        RootEntry current;
        for (int i = 0; i < rootEntries.size(); i++) {        //���ҵ���һ��Ŀ¼����ת����������˵
            string name = getName(rootEntries[i]);
            string extension;
            if (!(extension = getExtension(rootEntries[i])).empty()) {
                name += ".";
                name += extension;
            }
            if (name == hierarchicalDirectories[0]) {
                startCluster = rootEntries[i].DIR_FstClus;
                current = rootEntries[i];
                break;
            } else {
                startCluster = -1;
            }
        }
        if (startCluster == -1) {
            throw "File Not Exist!\n";
        }
        for (int i = 1; i < hierarchicalDirectories.size(); i++) {
            current = findCluster(hierarchicalDirectories[i], startCluster);
            startCluster = current.DIR_FstClus;    //����һ��Ŀ¼��ʼ�Ĵ���Ѱ�ҵ�ǰĿ¼��ʼ�Ĵ�
            if (startCluster == -1) { //��ǰĿ¼������
                throw "Wrong Path!\n";
            }
        }

        if(current.DIR_Attr != 0x20){
            throw "Wrong Path!\n";
        }
        int currentCluster = startCluster;
        char *content = new char[current.DIR_FileSize];
        int restSize = current.DIR_FileSize;
        int hasRead = 0;
        while (true) {
            fin.seekg(
                    (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16 + currentCluster - 2) * bpb.BPB_BytsPerSEC +
                    bpb.BPB_RootEntCnt * 32, ios::beg);

            if (restSize >= bpb.BPB_BytsPerSEC) {
                fin.read((content + hasRead), bpb.BPB_BytsPerSEC);
                hasRead += bpb.BPB_BytsPerSEC;
                restSize -= bpb.BPB_BytsPerSEC;
            } else {
                fin.read((content + hasRead), restSize);
                break;
            }
            currentCluster = calNextCluster(currentCluster);
        }
        string s = "";
        for (int i = 0; i < current.DIR_FileSize; i++) {
            s += content[i];
        }
        s += "\n";
        whitePrint(s.c_str());
        //cout << endl;
        delete[] content;
    }
}


void solveInput() {
    string command;
    //cout << ">";
    string s = ">";
    whitePrint(s.c_str());
    getline(cin, command);
    if (command.length() == 0) {
        return;
    }

    if (command == "exit") {
        // cout << "Exit Successfully!";
        whitePrint("Exit Successfully!\n");
        throw 1;
    }
    vector<string> splitedCommand = commandSplit(command);
    queue<string> q;  //����·��
    if (splitedCommand[0] == "ls") {
        if (splitedCommand.size() == 1) {
            q.push("/");
            ls(q);        //���κβ���
        } else {
            for (int i = 1; i < splitedCommand.size(); i++) {     //��-ll����ȫ���滻Ϊ-l����
//                if (splitedCommand[i] == "-ll") {
//                    splitedCommand[i] = "-l";
//                }
                string temp = splitedCommand[i];
                if(temp.substr(0, 1) == "-"){
                    if(temp.size() == 1){
                        throw "Wrong Command!\n";
                    }
                    for(int j = 1; j < temp.size(); j++){
                        if(temp.substr(j, 1) != "l"){
                            throw "Wrong Command\n";
                        }
                    }
                    splitedCommand[i] = "-l";
                }
            }

            string path = "";
            int countOfPath = 0;
            bool containL = false;
            for (int i = 1; i < splitedCommand.size(); i++) {
                if (splitedCommand[i] == "-l") {    //��ǰ����Ϊ"-l"
                    containL = true;
                } else {    //��ǰ����Ϊ�Ϸ�Ŀ¼��
                    path = splitedCommand[i];
                    countOfPath++;
                }
            }

            if (countOfPath > 1) {   //�����Ŀ¼������
                throw "Too Many Diretories\n";
            }

            if (containL) {
                if (countOfPath == 0) {    //���ļ�·����Ĭ��Ϊ��Ŀ¼
                    q.push("/");
                    lsl(q);
                } else {
                    q.push(path);
                    lsl(q);
                }
            } else {
                q.push(path);
                ls(q);
            }
        }
    } else if (splitedCommand[0] == "cat") {
        if (splitedCommand.size() >= 3) {
            throw "Too Many Parameters!\n";
        } else if (splitedCommand.size() == 1) {
            throw "Please Input File Path!\n";
        } else {
            cat(splitedCommand[1]);
        }
    } else {
        throw "Wrong Command!\n";
    }
    nextLevel.clear();
    toPrint.clear();
}

int main() {
    fin.open("../a.img", ios_base::binary | ios::in); //�Զ����ƶ�ȡa.img�ļ�
    if (!fin.is_open()) {
        //cout << "File Not Exist!" << endl;
        whitePrint("File Not Exist!\n");
    }

    //��ȡBPB
    fin.seekg(3, ios::beg);
    fin.read((char *) &bpb, sizeof(bpb));

    //��ȡfat1
    fat1 = new char[bpb.BPB_FATSz16 * bpb.BPB_BytsPerSEC];
    fin.seekg(bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSEC, ios::beg);
    fin.read(fat1, bpb.BPB_FATSz16 * bpb.BPB_BytsPerSEC);


    //��ȡRootEntry�����224��Ŀ¼��
    fin.seekg((bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16) * bpb.BPB_BytsPerSEC, ios::beg);
    for (int i = 0; i < bpb.BPB_RootEntCnt; i++) {
        fin.read((char *) &rootEntries[i], sizeof(RootEntry));
    }

    while (true) {
        try {
            solveInput();
        } catch (const char *s) {
            //cout << s << endl;
            whitePrint(s);
        } catch (int exitcode) {    //�յ��˳�������
            fin.close();
            return 1;
        }
    }
}


vector<string> commandSplit(string &command) {
    vector<string> splitedCommand = split(command, " ");
    return splitedCommand;
}

vector<string> split(string str, string pattern) {
    int pos;
    vector<string> result;
    str += pattern; //��չ�ַ����Է��������ȷ��pos��Ϊ-1
    int size = str.size();
    for (int i = 0; i < size; i++) {
        pos = str.find(pattern, i); //���±�i��ʼ�ҵ�һ��pattern
        if (pos < size) {
            string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}


void trim(string &s) {
    int index = 0;
    if (!s.empty()) {
        while ((index = s.find(" ", index)) != string::npos) {
            s.erase(index, 1);
        }
    }
}

void lsOut(vector<pair<string, char> > toPrint) {     //ÿ�չ�һ�����һ��
    for (int i = 0; i < toPrint.size(); i++) {
        pair<string, char> &p = toPrint[i];
        if (p.second == 'r') {
            //cout << "\033[31m" << p.first << "  " << "\033[0m";
            string s = p.first + "  ";
            redPrint(s.c_str());
        } else {
            //cout << "\033[31m" << p.first << "  " << "\033[0m";
            string s = p.first + "  ";
            whitePrint(s.c_str());
        }
    }
    //cout << endl;
    whitePrint("\n");
}


string getName(RootEntry rootEntry) {
    string res = "";
    for (int i = 0; i < 8; i++) {
        if (rootEntry.DIR_Name[i] != ' ') {
            res += rootEntry.DIR_Name[i];
        }
    }
    return res;
}

string getExtension(RootEntry rootEntry) {
    string res = "";
    for (int i = 8; i < 11; i++) {
        if (rootEntry.DIR_Name[i] != ' ') {
            res += rootEntry.DIR_Name[i];
        }
    }
    return res;
}




