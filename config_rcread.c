/* Generated by genconfig */
			else if(strcmp(cmd, "no-mcc")==0)
				mirc_colour_compat=false;
			else if(strcmp(cmd, "mcc")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					mirc_colour_compat=value;
				else
				{
					asb_failsafe(c_err, "Malformed rc entry for mcc (value not numeric)");
					asb_failsafe(c_err, rest);
				}
			}
			else if(strcmp(cmd, "no-fred")==0)
				force_redraw=false;
			else if(strcmp(cmd, "fred")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					force_redraw=value;
				else
				{
					asb_failsafe(c_err, "Malformed rc entry for fred (value not numeric)");
					asb_failsafe(c_err, rest);
				}
			}
			else if(strcmp(cmd, "buf")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					buflines=value;
				else
				{
					asb_failsafe(c_err, "Malformed rc entry for buf (value not numeric)");
					asb_failsafe(c_err, rest);
				}
			}
			else if(strcmp(cmd, "mnln")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					maxnlen=value;
				else
				{
					asb_failsafe(c_err, "Malformed rc entry for mnln (value not numeric)");
					asb_failsafe(c_err, rest);
				}
			}
			else if(strcmp(cmd, "no-fwc")==0)
				full_width_colour=false;
			else if(strcmp(cmd, "fwc")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					full_width_colour=value;
				else
					full_width_colour=true;
			}
			else if(strcmp(cmd, "no-hts")==0)
				hilite_tabstrip=false;
			else if(strcmp(cmd, "hts")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					hilite_tabstrip=value;
				else
					hilite_tabstrip=true;
			}
			else if(strcmp(cmd, "no-tsb")==0)
				tsb=false;
			else if(strcmp(cmd, "tsb")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					tsb=value;
				else
					tsb=true;
			}
			else if(strcmp(cmd, "tping")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					tping=value;
				else
				{
					asb_failsafe(c_err, "Malformed rc entry for tping (value not numeric)");
					asb_failsafe(c_err, rest);
				}
			}
			else if(strcmp(cmd, "no-ts")==0)
				ts=false;
			else if(strcmp(cmd, "ts")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					ts=value;
				else
				{
					asb_failsafe(c_err, "Malformed rc entry for ts (value not numeric)");
					asb_failsafe(c_err, rest);
				}
			}
			else if(strcmp(cmd, "no-utc")==0)
				utc=false;
			else if(strcmp(cmd, "utc")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					utc=value;
				else
					utc=true;
			}
			else if(strcmp(cmd, "no-quiet")==0)
				quiet=false;
			else if(strcmp(cmd, "quiet")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					quiet=value;
				else
					quiet=true;
			}