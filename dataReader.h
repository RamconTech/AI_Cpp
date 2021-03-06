#ifndef _DATAREADER
#define _DATAREADER

//standard libraries
#include <vector>
#include <fstream>
#include <string>
#include <math.h>
#include <algorithm>

//custom includes
#include "dataEntry.h"

using namespace std;

//dataset retrieval approach enum
enum { NONE, STATIC, GROWING, WINDOWING };

class dataSet
{
public:

	vector<dataEntry*> trainingSet;
	vector<dataEntry*> generalizationSet;
	vector<dataEntry*> validationSet;

	dataSet()//Constructor 
	{		
	}

	~dataSet()//Destructor
	{
		trainingSet.clear();
		generalizationSet.clear();
		validationSet.clear();
	}

	void clear()
	{
		trainingSet.clear();
		generalizationSet.clear();
		validationSet.clear();
	}
};

//data reader class
class dataReader
{
//private members
//----------------------------------------------------------------------------------------------------------------
private:
	//data storage
	vector<dataEntry*> Data;
	int nInputs;
	int nTargets;

	//current data set
	dataSet DataSet;

	//data set creation approach and total number of dataSets
	int creationApproach;
	int numDataSets;
	int trainingDataEndIndex;

	//creation approach variables
	double growingStepSize;			//step size - percentage of total set
	int growingLastDataIndex;		//last index added to current dataSet
	int windowingSetSize;			//initial size of set
	int windowingStepSize;			//how many entries to move window by
	int windowingStartIndex;		//window start index	
	
//public methods
//----------------------------------------------------------------------------------------------------------------
public:

	//constructor
	dataReader(): creationApproach(NONE), numDataSets(-1)
	{				
	}

	//destructor
	~dataReader()
	{
		//clear data
		for (int i=0; i < (int) Data.size(); i++ ) delete Data[i];		
		Data.clear();		 
	}

	//read data from file
	bool loadDataFile( const char* filename, int nI, int nT )
	{
		//clear any previous data
		for (int i=0; i < (int) Data.size(); i++ ) delete Data[i];		
		Data.clear();
		DataSet.clear();
		
		//set number of inputs and outputs
		nInputs = nI;
		nTargets = nT;

		//open file for reading
		fstream inputFile;
		inputFile.open(filename, ios::in);	

		if (inputFile.is_open())
		{
			string line = "";
			
			//read data
			while ( !inputFile.eof() )
			{
				getline(inputFile, line);				
				
				//process line
				if (line.length() > 2 ) processLine(line);
			}		
			
			//shuffle data
			random_shuffle(Data.begin(), Data.end());

			//split data set
			trainingDataEndIndex = (int) (0.6 * Data.size());
			int gSize = (int) ( ceil(0.2 * Data.size()) );
			int vSize = (int) ( Data.size() - trainingDataEndIndex - gSize );
								
			//generalization set
			for (int i = trainingDataEndIndex; i < trainingDataEndIndex + gSize; i++)
			{
				DataSet.generalizationSet.push_back(Data[i]);
			}
					
			//validation set
			for (int i = trainingDataEndIndex + gSize; i < (int)Data.size(); i++)
			{
				DataSet.validationSet.push_back(Data[i]);
			}
			
			//print success
			cout << "Data File Read Complete >> Patterns Loaded: " << Data.size() << endl;

			//close file
			inputFile.close();
			
			return true;
		}
		else return false;
	}

	//select dataset approach
	void setCreationApproach( int approach, double param1 = -1, double param2 = -1 )
	{
		//static
		if (approach == STATIC)
		{
			creationApproach = STATIC;
			
			//only 1 data set
			numDataSets = 1;
		}

		//growing
		else if ( approach == GROWING )
		{			
			if ( param1 <= 100.0 && param1 > 0)
			{
				creationApproach = GROWING;
			
				//step size
				growingStepSize = param1 / 100;
				growingLastDataIndex = 0;

				//number of sets
				numDataSets = (int) ceil( 1 / growingStepSize );				
			}
		}

		//windowing
		else if ( approach == WINDOWING )
		{
			//if initial size smaller than total entries and step size smaller than set size
			if (param1 < Data.size() && param2 <= param1)
			{
				creationApproach = WINDOWING;
				
				//params
				windowingSetSize = (int) param1;
				windowingStepSize = (int) param2;
				windowingStartIndex = 0;			

				//number of sets
				numDataSets = (int) ceil( (double) ( trainingDataEndIndex - windowingSetSize ) / windowingStepSize ) + 1;
			}			
		}

	}

	//return number of datasets
	int nDataSets()
	{
		return numDataSets;
	}
	
	dataSet* getDataSet()
	{		
		switch ( creationApproach )
		{
		
			case STATIC : createStaticDataSet(); break;
			case GROWING : createGrowingDataSet(); break;
			case WINDOWING : createWindowingDataSet(); break;
		}
		
		return &DataSet;
	}

//private methods
//----------------------------------------------------------------------------------------------------------------
private:
	
	void createStaticDataSet()
	{
		//training set
		for (int i = 0; i < trainingDataEndIndex; i++)
		{
			DataSet.trainingSet.push_back(Data[i]);
		}
	}

	void createGrowingDataSet()
	{
		//increase data set by step percentage
		growingLastDataIndex += (int) ceil(growingStepSize * trainingDataEndIndex);		
		if ( growingLastDataIndex > (int) trainingDataEndIndex ) growingLastDataIndex = trainingDataEndIndex;

		//clear sets
		DataSet.trainingSet.clear();
		
		//training set
		for (int i = 0; i < growingLastDataIndex; i++)
		{
			DataSet.trainingSet.push_back(Data[i]);
		}
	}

	void createWindowingDataSet()
	{
		//create end point
		int endIndex = windowingStartIndex + windowingSetSize;
		if ( endIndex > trainingDataEndIndex ) endIndex = trainingDataEndIndex;		

		//clear sets
		DataSet.trainingSet.clear();
						
		//training set
		for ( int i = windowingStartIndex; i < endIndex; i++ ) DataSet.trainingSet.push_back( Data[i] );
				
		//increase start index
		windowingStartIndex += windowingStepSize;
	}
	
	void processLine( string &line )
	{
		//create new pattern and target
		double* pattern = new double[nInputs];
		double* target = new double[nTargets];
		
		//store inputs		
		char* cstr = new char[line.size()+1];
		char* t;
		strcpy(cstr, line.c_str());

		//tokenise
		int i = 0;
		t=strtok (cstr,",");
		
		while ( t!=NULL && i < (nInputs + nTargets) )
		{	
			if ( i < nInputs ) pattern[i] = atof(t);
			else target[i - nInputs] = atof(t);

			//move token onwards
			t = strtok(NULL,",");
			i++;			
		}
		
		/*cout << "pattern: ";
		for (int i=0; i < nInputs; i++) 
		{
			cout << pattern[i] << ",";
		}
		
		cout << " target: ";
		for (int i=0; i < nTargets; i++) 
		{
			cout << target[i] << " ";
		}*/
		cout << endl;


		//add to records
		Data.push_back( new dataEntry( pattern, target ) );		
	}
};

#endif
