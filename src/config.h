FILE* configFile=NULL;



bool config_open(const char* filename)
{
	configFile=fopen(filename,"rt");

	return configFile?true:false;
}



void config_close(void)
{
	if(configFile)
	{
		fclose(configFile);
		configFile=NULL;
	}
}



int config_read_int(const char* name,int val)
{
	char param[1024],line[1024];

	strcpy(param,name);
	strcat(param,"=");

	if(configFile)
	{
		fseek(configFile,0,SEEK_SET);

		while(fgets(line,sizeof(line),configFile))
		{
			if(strstr(line,param))
			{
				sscanf(&line[strlen(param)],"%i",&val);
			}
		}
	}

	return val;
}



const char* config_read_string(const char* name,const char* str)
{
	static char param[1024];
	char line[1024];

	strcpy(param,name);
	strcat(param,"=");

	if(configFile)
	{
		fseek(configFile,0,SEEK_SET);

		while(fgets(line,sizeof(line),configFile))
		{
			if(strstr(line,param))
			{
				sscanf(&line[strlen(param)],"%1023[^\n]",&param);
				return param;
			}
		}
	}

	return str;
}