#include "Main.cpp"


int main(int argc, char const *argv[]) {
    (void) argc;
    (void) argv;

    cout << "Welcome to the Log File Simulator. Let me just initalize some functionalities. Go get a drink while you wait...shouldn't be too long" << endl;

    createDrive();
    // loadFromCheckpoint();
    // readFileMap();

    cout << "Done! lets get started" << endl;
    cout << "\nuserspace// ";
    string input;
    vector<string> result;

    while(getline(cin, input)){
        //Getting user input
        result.clear();
        if(!input.empty()){
            istringstream stream(input);
            string line;
            while(getline(stream, line, ' ')){
                result.push_back(line);
            }

            if(result[0] == "import" && result.size() == 3){
                import(result[1], result[2]);

            }else if(result[0] == "remove" && result.size() == 2){
                remove(result[1]);
                //do remove;
            }else if(result[0] == "cat" && result.size() == 2){
                cat(result[1]);
            }else if(result[0] == "display" && result.size() == 4){
                int howmany = stoi(result[2]);
                int start = stoi(result[3]);
                display(result[1], howmany, start);
            }else if(result[0] == "overwrite" && result.size() == 5){
                int howmany = stoi(result[2]);
                int start = stoi(result[3]);
                const char* param4 = result[4].c_str();
                char c = param4[0];
                overwrite(result[1], howmany, start, c);
            }else if(result[0] == "list" && result.size() == 1){
                list();
            }else if(result[0] == "shutdown" && result.size() == 1){
                initalizeShutdown();
                writer.writeSegmentToDisk();
                exit(EXIT_SUCCESS);

            }else if(result[0] == "usage" && result.size() == 1){
                cout << "";
            }else{
                cout << "INVALID COMMAND! TRY AGAIN" << endl;
            }


        }
        cout<< "userspace// ";
    }


    return 0;
}
