/*
*
* Program: Pearson Correlatrion Coefficient computation.
* Author: Andrea Purgato
* Version: counter occurences version.
*
* File: Manager.cu
* Description: file with alle the functions used to communicate with the input and output files.
*
*/

#pragma once

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Logger.cpp"
//#include "DevicesReader.cu"

// ### Info file variables.
std::string INFO_NAME = "info.txt";
std::string INFO_FOLDER = "data/";

// ### Data file variables.
std::string DATA_NAME = "values.txt";
std::string DATA_FOLDER = "data/";

// ### Results file variables.
std::string RES_FOLDER = "output/";

// ###Struct containing the images info.
struct info{
	int nodeNumber;
	int frameNumber;
};

/*
	### Function:
	Set the files name.
*/
void setFilesName(std::string info, std::string data){

	INFO_NAME = info;
	DATA_NAME = data;
	log("Info file: " + info + ", Data file: " + data);

}

/*
	### Function:
	Return the node info.
*/
info getInfo() {

	info i;

	std::string str, aux;
	std::string delimiter = ",";

	/*
		Nodes info.
	*/

	/// Open the info file.
	std::ifstream iFile(INFO_FOLDER + INFO_NAME);

	/// Nodes Number
	std::getline(iFile, str);
	str.erase(0, str.find(delimiter) + delimiter.length());
	aux = str.substr(0, str.find(delimiter));
	i.nodeNumber = std::stoi(aux);

	/// Frames Number
	std::getline(iFile, str);
	str.erase(0, str.find(delimiter) + delimiter.length());
	aux = str.substr(0, str.find(delimiter));
	i.frameNumber = std::stoi(aux);

	/// Print information.
	log("Total pixel number: " + std::to_string(i.nodeNumber));
	log("Images number: " + std::to_string(i.frameNumber));

	return i;
}

/*
	### Function:
	Read all the data from the txt file.
*/
int* getData(info nodeInfo){

	std::cout << "Loading data from file\n";

	/// Allocate the meomory in RAM.
	int *data = (int*)malloc(nodeInfo.nodeNumber * nodeInfo.frameNumber * sizeof(int));
	log("Memory needed for the data: " + std::to_string(nodeInfo.nodeNumber * nodeInfo.frameNumber * sizeof(int) / 1000000) + " MB");

	/// Open the data file.
	std::string str, aux;
	std::string delimiter = ",";

	std::ifstream iFile(DATA_FOLDER + DATA_NAME);

	/// Read the data inside.
	for (int ts = 0; ts < nodeInfo.frameNumber; ts++) {

		/// Read the file line
		std::getline(iFile, str);

		/// Remove the time stamp.
		str.erase(0, str.find(delimiter) + delimiter.length());

		int idx = 0;
		/// Split the line and take the values for all the images.
		for (int p = 0; p < nodeInfo.nodeNumber; p++){

			idx = ts + p * nodeInfo.frameNumber;

			/// Take the value
			aux = str.substr(0, str.find(delimiter));
			str.erase(0, str.find(delimiter) + delimiter.length());

			/// Store in the data structure.
			data[idx] = std::stoi(aux);

		}

	}

	return data;

}


/*
	### Functions:
	Compute the starting pixel and the ending pixel.
*/
int getStartingPixel(int device, info imagesInfo, int gpuNumber){

	if (device == 0)
		return 0;
	else
		return std::floor((float)imagesInfo.nodeNumber / gpuNumber) * device + 1;

}
int getEndingPixel(int device, info imagesInfo, int gpuNumber){

	return getStartingPixel(device, imagesInfo, gpuNumber) + std::floor((float)imagesInfo.nodeNumber / gpuNumber);

}

/*
	### Function:
	save all the data into a txt file.

	This function will be launched by a thread that will save the file indipendently.
*/
void saveResults(float *saveData, int time, info nodeInfo, int TIME_WINDOW) {

	/// Name of the file to save.
	std::string fileName_low;
	std::string fileName_high;

	/// Create the file name.
	fileName_low = "N_" + std::to_string(nodeInfo.nodeNumber) + "_W_" + std::to_string(TIME_WINDOW) + "_T_" + std::to_string(time) + "_correlation.txt";
	fileName_high = "N_" + std::to_string(nodeInfo.nodeNumber) + "_W_" + std::to_string(TIME_WINDOW) + "_T_" + std::to_string(time + 1) + "_correlation.txt";
	log("SAVING THREAD, start saving file " + fileName_low + " and " + fileName_high);

	/// Open the file.
	std::ofstream oFileLow(RES_FOLDER + fileName_low);
	std::ofstream oFileHigh(RES_FOLDER + fileName_high);

	/// Write the results.
	std::string str_low = "";
	std::string str_high = "";
	for (int r = 0; r < nodeInfo.nodeNumber; r++){

		for (int c = r; c < nodeInfo.nodeNumber; c++){

			int result_index = (r * nodeInfo.nodeNumber + c);

			if (r < c) {

				/// TIME T
				str_low.append(std::to_string(r) + "," + std::to_string(c) + "," + std::to_string(saveData[result_index]) + "\n");

			} else if (r > c) {

				/// TIME (T + 1)
				str_high.append(std::to_string(r) + "," + std::to_string(c) + "," + std::to_string(saveData[result_index]) + "\n");

			}			

		}

	}

	oFileLow << str_low;
	oFileLow.close();

	oFileLow << str_high;
	oFileHigh.close();

	log("SAVING THREAD, end saving file " + fileName_low + " and " + fileName_high);

	/// Free the memory with the results.
	free(saveData);

}

/*
	### Function:
	save the results that are split in different quadrant in a txt file.
	- The function merege alone the results of each quadrant.

	This function is launched as indipendent thread.
*/
void saveQuadrantResults(float *saveData, int time, info quadrantInfo, int TIME_WINDOW, int xOffset, int yOffset, int N){

	/// Name of the file to save.
	std::string fileName_low;
	std::string fileName_high;

	/// Create the file name.
	fileName_low = "N_" + std::to_string(N) + "_W_" + std::to_string(TIME_WINDOW) + "_T_" + std::to_string(time) + "_correlation.txt";
	fileName_high = "N_" + std::to_string(N) + "_W_" + std::to_string(TIME_WINDOW) + "_T_" + std::to_string(time + 1) + "_correlation.txt";
	log("SAVING THREAD, start saving file " + fileName_low + " and " + fileName_high);

	/// Open the file.
	std::ofstream oFileLow(RES_FOLDER + fileName_low, std::ios_base::app);
	std::ofstream oFileHigh(RES_FOLDER + fileName_high, std::ios_base::app);

	/// Write the results.
	std::string str_low = "";
	std::string str_high = "";
	for (int r = 0; r < quadrantInfo.nodeNumber; r++){

		for (int c = 0; c < quadrantInfo.nodeNumber; c++){

			int result_index = (r * quadrantInfo.nodeNumber + c);

			if ((r + yOffset) < (c + xOffset)) {

				/// TIME T
				str_low.append(std::to_string(r) + "," + std::to_string(c) + "," + std::to_string(saveData[result_index]) + "\n");

			} else if ((r + yOffset) > (c + xOffset)) {

				/// TIME (T + 1)
				str_high.append(std::to_string(r) + "," + std::to_string(c) + "," + std::to_string(saveData[result_index]) + "\n");

			}

		}
	}

	oFileLow << str_low;
	oFileLow.close();

	oFileLow << str_high;
	oFileHigh.close();

	log("SAVING THREAD, end saving file " + fileName_low + " and " + fileName_high);

	/// Free the memory with the results.
	free(saveData);

}
