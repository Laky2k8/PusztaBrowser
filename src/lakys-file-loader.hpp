#include <iostream>
#include <fstream>
#include <string>

using namespace std;

string load_file(string file_path)
{

	// Remove / from the beginning of the file path
	if(file_path[0] == '/')
	{
		file_path = file_path.substr(1);
	}

	cout << "Loading " << file_path << endl;

	string full_text;
	string line;

	ifstream file(file_path);

	while(getline(file, line))
	{
		std::cout << "Line: " << line << endl;
		full_text += (line + "\n");
	}

	std::cout << "Full text: " << full_text << endl;

	file.close();

	return full_text;
}

bool save_to_file(string file_path, string content)
{
		// Remove / from the beginning of the file path
	if(file_path[0] == '/')
	{
		file_path = file_path.substr(1);
	}

	cout << "Saving to " << file_path << endl;

	ofstream file(file_path);
	if(!file.is_open())
	{
		cout << "Failed to open file for writing: " << file_path << endl;
		return false;
	}

	file << content;
	file.close();

}