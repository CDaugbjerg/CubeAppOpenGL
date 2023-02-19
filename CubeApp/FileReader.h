#include<fstream>
#include <string>

using namespace std;

bool ReadFileToString(const char* pFileName, string& outFile)
{
    ifstream f(pFileName);

    bool ret = false;

    if (f.is_open()) {
        string line;
        while (getline(f, line)) {
            outFile.append(line);
            outFile.append("\n");
        }

        f.close();

        ret = true;
    }
    else
    {
        fprintf(stderr, " ReadFileToString Error: '%s'\n", pFileName);

    }

    return ret;
}