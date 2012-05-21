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
					atr_failsafe(&s_buf, ERR, "Malformed rc entry for mcc (value not numeric)", "init: ");
					atr_failsafe(&s_buf, ERR, rest, "init: ");
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
					atr_failsafe(&s_buf, ERR, "Malformed rc entry for fred (value not numeric)", "init: ");
					atr_failsafe(&s_buf, ERR, rest, "init: ");
				}
			}
			else if(strcmp(cmd, "buf")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					buflines=value;
				else
				{
					atr_failsafe(&s_buf, ERR, "Malformed rc entry for buf (value not numeric)", "init: ");
					atr_failsafe(&s_buf, ERR, rest, "init: ");
				}
			}
			else if(strcmp(cmd, "mnln")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					maxnlen=value;
				else
				{
					atr_failsafe(&s_buf, ERR, "Malformed rc entry for mnln (value not numeric)", "init: ");
					atr_failsafe(&s_buf, ERR, rest, "init: ");
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
					atr_failsafe(&s_buf, ERR, "Malformed rc entry for tping (value not numeric)", "init: ");
					atr_failsafe(&s_buf, ERR, rest, "init: ");
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
					atr_failsafe(&s_buf, ERR, "Malformed rc entry for ts (value not numeric)", "init: ");
					atr_failsafe(&s_buf, ERR, rest, "init: ");
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
			else if(strcmp(cmd, "no-its")==0)
				its=false;
			else if(strcmp(cmd, "its")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					its=value;
				else
					its=true;
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
			else if(strcmp(cmd, "no-debug")==0)
				debug=false;
			else if(strcmp(cmd, "debug")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					debug=value;
				else
					debug=true;
			}
			else if(strcmp(cmd, "no-conf")==0)
				conf=false;
			else if(strcmp(cmd, "conf")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					conf=value;
				else
					conf=true;
			}
			else if(strcmp(cmd, "no-prefix")==0)
				show_prefix=false;
			else if(strcmp(cmd, "prefix")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					show_prefix=value;
				else
					show_prefix=true;
			}
			else if(strcmp(cmd, "no-titles")==0)
				titles=false;
			else if(strcmp(cmd, "titles")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					titles=value;
				else
					titles=true;
			}
			else if(strcmp(cmd, "no-winch")==0)
				winch=false;
			else if(strcmp(cmd, "winch")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					winch=value;
				else
					winch=true;
			}
			else if(strcmp(cmd, "no-indent")==0)
				indent=false;
			else if(strcmp(cmd, "indent")==0)
			{
				unsigned int value;
				if(rest&&sscanf(rest, "%u", &value))
					indent=value;
				else
					indent=true;
			}
