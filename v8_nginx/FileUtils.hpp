#include <fstream>
using namespace std;


//  该类用于处理文件的辅助函数
class FileUtils {
public:

    static string readfile(string file_path) {
        ifstream ifs;
        ifs.open(file_path,ios::binary | ios::in);
        if(!ifs.is_open()) {
            return nullptr;
        }

        //  得到buf
        //  获取文件长度
        //  seekg(0,ios::end) 表示将文件指针自end位置偏移0字节--也就是置于文件尾
        ifs.seekg(0,ios::end);
        //  tellg() 表示从文件指针的位置到文件头位置的字节数
        size_t length = ifs.tellg();
        //  再将文件指针复位至文件开头以正常操作文件
        ifs.seekg(0,ios::beg);

        char buf[length];
        ifs.read(buf,length);
        return string(buf); 
    }




};