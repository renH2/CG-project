#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>

using namespace std;
#define MAX 999;
#define MIN -999;

void mytrimE(char *s);
void mytrimF(char *s);

int main()
{
	float ViewMinX = MAX;
	float ViewMinY = MAX;
	float ViewMinZ = MAX;
	float ViewMaxX = MIN;
	float ViewMaxY = MIN;
	float ViewMaxZ = MIN;
	float temp;
	int  i;
	char content[1024];
	ifstream F("../../monsters/ogre/OgreOBJ.obj");
	if (!F.is_open())
	{
		cout << "Error in opening ObjFile\n";
		exit(1);
	}
	while (!F.eof())
	{
		memset(content, 0, sizeof(content));
		F.getline(content, 1024);
		mytrimF(content);
		mytrimE(content);
		switch (content[0])
		{
		case 'v':
			if (content[1] == ' ')
			{
				strtok(content, " ");
				for (i = 0; i < 3; i++)
				{
					temp = atof(strtok(NULL, " "));
					switch (i)
					{
					case 0:
						if (temp < ViewMinX)
							ViewMinX = temp;
						if (temp > ViewMaxX)
							ViewMaxX = temp;
						break;
					case 1:
						if (temp < ViewMinY)
							ViewMinY = temp;
						if (temp > ViewMaxY)
							ViewMaxY = temp;
						break;
					case 2:
						if (temp < ViewMinZ)
							ViewMinZ = temp;
						if (temp > ViewMaxZ)
							ViewMaxZ = temp;
						break;
					default:
						break;
					}
				}
				break;
			}
		default:
			break;
		}
	}
	F.close();
	cout << ViewMinX << " " << ViewMaxX << endl;
	cout << ViewMinY << " " << ViewMaxY << endl;
	cout << ViewMinZ << " " << ViewMaxZ << endl;
	return 0;
}

void mytrimF(char *s)
{
	char *t;
	t = s;
	while (*t == ' ' || *t == '\t')
		t++;
	while (*s++ = *t++);
}

void mytrimE(char *s)
{
	int  i = strlen(s) - 1;
	for (; i >= 0; i--)
		if (s[i] != '\t'&&s[i] != ' '&&s[i] != '\n')
			break;
	s[i + 1] = '\0';
}