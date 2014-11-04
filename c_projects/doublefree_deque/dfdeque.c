#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <deque>

struct STest
{
	char* pArray;
	int num1;
	int num2;
};

void DeleteSockData(STest *pData)
{ 
  if (pData)
  {
    if (pData->pArray)
    {
      free(pData->pArray);
    }
    delete pData;
  }
}

int main(int argc, char** argv)
{
	std::deque<struct STest> vrsta;
	
	for (int i=0; i < 5; i++)
	{
		STest sTest;
		sTest.pArray = (char*)malloc(sizeof(char)*5);
		sTest.pArray[0] = sTest.pArray[1] = sTest.pArray[2] = sTest.pArray[3] = sTest.pArray[4] = 100+i;
		sTest.num1 = i;
		sTest.num2 = i +10;
		vrsta.push_back(sTest);
	}
	
	std::cout << "deque filled " << vrsta.size()*sizeof(struct STest)*(sizeof(char)*5) << std::endl;
	
	struct STest *pData;
/*	pData = &vrsta.front();
	std::cout << "dobimo zacetek" << std::endl;
	vrsta.pop_front();
	std::cout << "pop_front" << std::endl;
	DeleteSockData(pData);
	*/
	while (vrsta.empty() == false)
	{
		pData = &vrsta.front();
		DeleteSockData(pData);
		vrsta.pop_front();
	}
	
	pData = &vrsta.front();
	std::cout << pData << std::endl;
	
	return 0;
}
