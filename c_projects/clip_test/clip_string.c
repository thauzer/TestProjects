#include "clip_string.h" 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *clipString(char *str)
{
	/*char	*begin = str;*/
	short	len = 0;
	char	*temp = NULL;
	//	printf("clipString1 %s \n", str);
	if (str == NULL)
		return(str);
	
	temp = str;
	//	printf("temp: %p, str: %p, *temp: %d\n", temp, str, *temp);
	while ((*temp == ' ') || (*temp == '\r') || (*temp == '\n') || (*temp == '\t'))
	{
		//		printf("clipString - remove from start\n");
		*temp = '\0';
		temp++;
	}
	str = temp;
	//	printf("clipString2 %s \n", str);
	len = strlen(str);
	if (len > 0)
	{
		//		printf("clipString - len > 0\n");
		temp = str + strlen(str) - 1;
		
		while ((*temp == ' ') || (*temp == '\r') || (*temp == '\n') || (*temp == '\t'))
		{
			//			printf("clipString - remove from end\n");
			*temp = '\0';
			temp--;
		}
	}
	printf("clipString3 %p - -->%s<-- \n", str, str);
	return(str);
}
