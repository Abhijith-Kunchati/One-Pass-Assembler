#ifdef __unix__

#elif defined(_WIN32) || defined(WIN32)

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <utility>
#include <stack>
#include <queue>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <math.h>

#endif

using namespace std;

inline bool fileExists(const string &name)
{
    ifstream f(name.c_str());
    return f.good();
}

class Assembler
{
private:
    map<string, string> OPTAB;
    map<string, pair<int, list<int>>> SYMTAB;
    map<int, pair<int, vector<string>>> records;
    string src_file_name;
    string symtab_file_name;
    string optab_file_name;
    string object_file_name;
    string listing_file_name;
    string header_record;
    string end_record;
    string program_name;
    int starting_address;
    int ending_address;
    int first_executable_instruction;
    int LOCCTR;
    int recordNo;

public:
    Assembler(string src, string optab, string symtab, string obj, string listing);
    void displaySourceCode();
    void displayOptab();
    void displayObjectCode();
    void populateOPTAB();
    void addRecord(string rec, bool createNewTextRecord);
    void generateObjectCode();
    void writeSYMTABFile();
    void writeObjectFile();
    vector<string> tokenize(string str);
    int hexToDec(string s);
    string decToHex(int a);
    string hexToBin(string s);
};

vector<string> Assembler::tokenize(string str)
{
    vector<string> tokens;
    string element;
    for (int i = 0; i < str.length(); i++)
    {
        if ((str[i] == ' ' || str[i] == '\t') && element.length() != 0)
        {
            tokens.push_back(element);
            element.clear();
        }
        else if (!(str[i] == ' ' || str[i] == '\t'))
        {
            element += string(1, str[i]);
        }
    }
    if (element.length() != 0)
    {
        tokens.push_back(element);
    }
    return tokens;
}

Assembler::Assembler(string src, string optab, string symtab, string obj, string listing)
{
    program_name = "      ";
    recordNo = 0;
    src_file_name = src;
    symtab_file_name = symtab;
    optab_file_name = optab;
    object_file_name = obj;
    listing_file_name = listing;
    populateOPTAB();
    generateObjectCode();
}

void Assembler::displayOptab()
{
    cout << endl;
    cout << "\t\t================================================\n";
    for (auto i : OPTAB)
    {
        cout << "\t\t" << i.first << "\t\t:\t\t" << i.second << endl;
    }
    cout << "\n\t\t================================================\n";
    return;
}

void Assembler::generateObjectCode()
{
    bool firstLine = true;
    string label, opcode, operand, LIST;
    ifstream sourceFile(src_file_name.c_str());
    ofstream listingFile(listing_file_name.c_str());
    for (string line; getline(sourceFile, line);)
    {
        vector<string> tokens = tokenize(line);
        int location = LOCCTR;

        //For First line of program
        if (firstLine)
        {
            firstLine = false;

            tokens[0] = tokens[0].substr(0, 6);
            program_name.replace(0, tokens[0].size(), tokens[0]);
            if (tokens.size() == 3)
            {
                if (tokens[1].compare(string("START")) == 0)
                {
                    LOCCTR = hexToDec(tokens[2]);
                    starting_address = LOCCTR;
                }
                else
                {
                    LOCCTR = 0;
                    starting_address = 0;
                }
            }
            else
            {
                LOCCTR = 0;
                starting_address = 0;
            }

            LIST = (string)decToHex(LOCCTR) + "    " + (string)line;
            listingFile << "Loc"
                        << "      Source Statement        "
                        << "             Object code" << endl;
            listingFile << endl
                        << LIST << endl;
            continue;
        }

        //FOR Last line of program
        if (tokens[0].compare("END") == 0)
        {
            ending_address = LOCCTR;

            if (tokens.size() > 1)
            {
                if (SYMTAB.find(tokens[1]) != SYMTAB.end())
                {
                    first_executable_instruction = SYMTAB[tokens[1]].first;
                }
                else
                {
                    cout << "\n\t\tUndefined Label : " << tokens[1] << endl;
                }
            }
            else
                first_executable_instruction = starting_address;

            LIST = "        " + (string)line;
            listingFile << LIST << endl;
            continue;
        }

        //comment line
        if (tokens[0].compare(".") == 0)
        {
            LIST = "        " + (string)line;
            listingFile << LIST << endl;
            continue;
        }

        if (tokens.size() == 1)
        {
            opcode = tokens[0];
        }
        if (tokens.size() == 2)
        {
            opcode = tokens[0];
            operand = tokens[1];
        }
        else if (tokens.size() == 3)
        {
            label = tokens[0];
            opcode = tokens[1];
            operand = tokens[2];
        }

        //handling label
        if (tokens.size() == 3)
        {
            if (SYMTAB.find(label) == SYMTAB.end())
            {
                SYMTAB.insert({label, pair<int, list<int>>(LOCCTR, {})});
            }
            else if (SYMTAB.find(label) != SYMTAB.end())
            {
                if (SYMTAB[label].first == -1)
                {
                    SYMTAB[label].first = LOCCTR;

                    for (auto i : SYMTAB[label].second)
                    {
                        addRecord(decToHex(i), true);
                    }
                }
                else
                {
                    cout << "\n\t\tDuplicate Label\n";
                    exit(0);
                }
            }
        }

        string newRecord = "000000";

        //handling opcode
        bool indexRegister = false;
        if (operand[operand.size() - 1] == 'X')
        {
            indexRegister = true;
            operand = operand.substr(0, operand.size() - 2);
        }

        if (OPTAB.find(opcode) != OPTAB.end())
        {
            newRecord.replace(0, 2, OPTAB[opcode]);
            if (tokens.size() > 1)
            {
                if (SYMTAB.find(operand) != SYMTAB.end())
                {
                    if (SYMTAB[operand].first != -1)
                    {
                        string addressFeild = decToHex(SYMTAB[operand].first);
                        newRecord.replace(6 - addressFeild.size(), 6, addressFeild);
                    }
                    else
                    {
                        SYMTAB[operand].second.push_back(LOCCTR);
                    }
                }
                else
                {
                    SYMTAB.insert({operand, pair<int, list<int>>(-1, {})});
                    SYMTAB[operand].second.push_back(LOCCTR);
                }
            }

            if (indexRegister)
            {
                string temp = "";
                temp += newRecord[2];
                temp = decToHex(hexToDec(temp) | 8);
                newRecord[2] = temp[0];
            }
            string Line = "                                      ";
            LIST = (string)decToHex(LOCCTR) + "    " + Line.replace(0, line.size(), line) + "    " + newRecord;
            listingFile << LIST << endl;
            addRecord(newRecord, false);
            LOCCTR += 3;
        }
        else
        {
            string constantValue = "";
            int newLocation = LOCCTR;

            if (opcode.compare(string("WORD")) == 0)
            {
                newLocation += 3;
                string temp = decToHex(stoi(operand));
                constantValue += string("000000").replace(6 - temp.size(), 6, temp);
            }
            else if (opcode.compare(string("RESW")) == 0)
            {
                LOCCTR += 3 * stoi(operand);
                recordNo++;
                continue;
            }
            else if (opcode.compare(string("BYTE")) == 0)
            {
                if (operand[0] == 'X')
                {
                    constantValue += operand[2];
                    constantValue += operand[3];
                    newLocation++;
                }
                else
                {
                    constantValue += decToHex((int)operand[2]);
                    constantValue += decToHex((int)operand[3]);
                    if (operand.size() == 6)
                    {
                        constantValue += decToHex((int)operand[4]);
                    }
                    newLocation += operand.length() - 3;
                }
            }
            else if (opcode.compare(string("RESB")) == 0)
            {
                LOCCTR += stoi(operand);
                recordNo++;
                continue;
            }
            else
            {
                cout << "\t\tError: Opcode : " << opcode << " is not present in OPTAB\n";
                exit(0);
            }
            newRecord.clear();
            newRecord = constantValue;
            if (indexRegister)
            {
                string temp = "";
                temp += newRecord[2];
                temp = decToHex(hexToDec(temp) | 8);
                newRecord[2] = temp[0];
            }
            string Line = "                                      ";
            LIST = (string)decToHex(LOCCTR) + "    " + Line.replace(0, line.size(), line) + "    " + newRecord;
            listingFile << LIST << endl;
            addRecord(newRecord, false);
            LOCCTR = newLocation;
        }
    }
    listingFile.close();
    sourceFile.close();

    writeSYMTABFile();
    writeObjectFile();
}

void assembleNewProgram()
{
    int inp;
    string src, optab, object, symtab, listing;
    cout << "\n\t\tSource File Name  :  ";
    cin >> src;
    cout << "\t\tFile name where Object Code will be stored  :  ";
    cin >> object;
    symtab = "symtab.txt";
    optab = "optab.txt";
    listing = "assemblerlisting.txt";

    if (!fileExists(src) || !fileExists(optab))
    {
        cout << "\n\t\tSOURCE FILE DOESN'T EXISTS\n\n";
        system("timeout 3");
        return;
    }

    Assembler newProgram(src, optab, symtab, object, listing);
    system("cls");

    while (1)
    {
        cout << "\n\n\t\t1. Display source code\n";
        cout << "\t\t2. Display OPTAB\n";
        cout << "\t\t3. Display object code\n";
        cout << "\t\t4. Return to Main\n";
        cout << " \n\t\tEnter your choice : ";
        cout << "\t\t\t\t\t";

        cin >> inp;

        if (inp == 4)
            break;
        switch (inp)
        {
        case 1:
            newProgram.displaySourceCode();
            break;
        case 2:
            newProgram.displayOptab();
            break;
        case 3:
            newProgram.displayObjectCode();
            break;

        default:
            cout << "\t\tEnter valid choice\n\n";
            system("timeout 1");
            break;
        }
    }
}
