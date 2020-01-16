#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <set>
#include <exception>
#endif

#include <sys/types.h>
#include <string>
#include <chrono>

using namespace std;

#ifdef _MSC_VER // 
int fork () {
	return 0;
}

int getpid () {
	return rand() % 200;
}

int getppid () {
	return 8;
}
#endif

void read_params_from_xml (int &nFiles, int &filesize, string &in_prefix, string &out_prefix)
{
	//should be reading from xml
#ifdef _MSC_VER
	nFiles = 1;
	filesize = 20000;
	in_prefix = "file";
	out_prefix = "out";
#else
	// Create empty property tree object
    boost::property_tree:pt::ptree tree;

    // Parse the XML into the property tree.
    boost::property_tree:pt::read_xml(filename, tree);

	// Get values from the tree
	nFiles = tree.get("config.num_processes", 0);
	filesize = tree.get("config.filesize", 0);
	in_prefix = tree.get<string>("config.in_prefix");
	out_prefix = tree.get<string>("config.out_prefix");
#endif
}

void generate_files (int nFiles, int filesize, string &in_prefix)
{
	for (int i = 0; i < nFiles; ++i) {
		string fname = in_prefix + to_string(i);
		FILE *fo = fopen(fname.data(), "wb");
		int symbols_left = filesize;
		while (symbols_left > 0)
		{
			// we want to avoid space/tab/newline in the end of the file.
			if (symbols_left <= 9) { // 9 here as our usual words are 4 to 8 symbols in length + 1 for space.
				for (int j = 0; j < symbols_left; ++j)
				{
					fputc(rand() % 26 + 'a', fo);
				}
				symbols_left = 0;
				break;
			}

			// generating word
			int wordlen = rand() % 4 + 4; // 4 to 8
			for (int j = 0; j < wordlen; ++j)
			{
				fputc(rand() % 26 + 'a', fo);
			}
			fputc(' ', fo);

			symbols_left -= wordlen + 1;
		}
		fclose(fo);
	}
}

int childish_work (int index, int filesize, string &in_prefix, int &tm)
{
	auto begin = chrono::high_resolution_clock::now();

	string fname = in_prefix + to_string(index);
	FILE *fi = fopen(fname.data(), "rb");
	// we assume that there can't be not empty file containing only spaces/tabs/newlines
	int nWords = (filesize > 0)? 1 : 0;
	// we assume that number of words = number of spaces/tabs/newlines + 1.
	for (int i = 0; i < filesize; ++i)
	{
		char c = fgetc(fi);
		if ((c == ' ') || (c == '\n') || (c == '\t')) 
		{
			++nWords;
		}
	}
	fclose(fi);
	auto end = chrono::high_resolution_clock::now();
	auto dur = end - begin;
	tm = chrono::duration_cast<chrono::microseconds>(dur).count();
	return nWords;
}

typedef int pid_t;

void fork_work (int nFiles, int filesize, string &in_prefix, string &out_prefix)
{
	for (int i = 0; i < nFiles; ++i) 
	{
		if (fork() == 0) // child
		{
			int time;
			int ret = childish_work (i, filesize, in_prefix, time);
			pid_t my_pid = getpid();
			pid_t parent_pid = getppid();
			string fname = out_prefix + to_string(i);
			FILE *fo = fopen (fname.data(), "wb");
			fprintf(fo, "file number %d:\n" "nWords = %d\n" "pid = %d\n" "ppid = %d\n" "time = %d" "us\n", i, ret, my_pid, parent_pid, time);
			fclose(fo);
			exit(0);
		}
		else // parent
		{
			continue;
		}
	}
}

int main ()
{
	//doing with arguments
	int nFiles;
	int filesize;
	string in_prefix, out_prefix;

	read_params_from_xml(nFiles, filesize, in_prefix, out_prefix);
	generate_files(nFiles, filesize, in_prefix);

	// fork + work
	fork_work(nFiles, filesize, in_prefix, out_prefix);

	return 0;
}