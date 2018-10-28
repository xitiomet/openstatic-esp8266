
//codeblock for files
const char *INDEX_MIN_JS = "function getXmlHttp() {var xmlhttp;if(window.XMLHttpRequest){xmlhttp=new XMLHttpRequest();}else if(window.ActiveXObject){xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");}else{alert(\"Your browser does not support XMLHTTP!\");} return xmlhttp;} function handleResponse(response) {var out=\"<table style=\\\"font-size: 26px;\\\" cellpadding=\\\"2\\\" cellspacing=\\\"0\\\">\";for(var key in response) {var display_value=response[key];if(!key.startsWith(\"_\")) {out+=\"<tr><td style=\\\"background-color: white;\\\">\"+key+\"</td><td style=\\\"padding-left: 14px; text-align: right;\\\">\"+display_value+\"</td></tr>\";}} out+=\"</table>\";document.getElementById('table_display').innerHTML=out;document.getElementById('clock').innerHTML=response._time;} function dataRefresh() {xmlhttp=getXmlHttp();xmlhttp.onreadystatechange=function() {if(xmlhttp.readyState==4) {var rtext=xmlhttp.responseText;if(rtext!='') {var response=eval('('+rtext+')');handleResponse(response);} setTimeout(\"dataRefresh()\",3000);}};xmlhttp.open(\"POST\",\"api.json?rnd=\"+Math.random(10000),true);xmlhttp.send(null);}\n";
const char *INDEX_MIN_HTML = "<html><head><title><%=hostname%></title><script src=index.js></script><style>body{background-color:#fff;font-family:Arial,Helvetica,Sans-Serif;color:#008;margin:0}</style></head><body onload=dataRefresh()><div style=display:block;background-color:#000;color:#fff;margin:0;padding:2px;width:100%><table style=width:99%;border:0;margin:0;padding:0 cellspacing=0 cellpadding=0><tr><td align=left><span style=font-size:22px;color:#fff><%=hostname%></span></td><td align=right><span id=clock style=color:#fff;padding-right:5px></span><input type=button value=settings onclick=\"window.location='/setup.html';\"></td></tr></table></div><div style=padding:2px id=table_display></div></body></html>\n";
const char *SETUP_MIN_HTML = "<html><head><title><%=hostname%></title><script src=index.js></script><style>body{background-color:#fff;font-family:Arial,Helvetica,Sans-Serif;color:#008;margin:0}</style></head><body onload=dataRefresh()><div style=display:block;background-color:#000;color:#fff;margin:0;padding:2px;width:100%><table style=width:99%;border:0;margin:0;padding:0 cellspacing=0 cellpadding=0><tr><td align=left><span style=font-size:22px;color:#fff><%=hostname%> - settings</span></td><td align=right><input type=button value=home onclick=\"window.location='/';\"></td></tr></table></div><form method=post action=/setup.html><div style=padding:2px><table><tr><td>Wifi Mode</td><td><select name=wifi_mode id=wifi_mode> <option value=0 <%=wifi_mode_off.selected%>>Wifi Off</option> <option value=1 <%=wifi_mode_station.selected%>>Station Mode</option> <option value=2 <%=wifi_mode_access.selected%>>Access Point Mode</option></select></td></tr><tr><td>Wifi Network</td><td><input name=ssid size=64 value=\"<%=ssid%>\"></td></tr><tr><td>Wifi Password</td><td><input type=password name=password size=64 value=\"<%=password%>\"></td></tr><tr><td>Hostname</td><td><input name=hostname size=64 value=\"<%=hostname%>\"></td></tr><tr><td>Push URL</td><td><input name=push_url size=64 value=\"<%=push_url%>\"></td></tr><tr><td>Push Interval</td><td><input name=push_interval size=3 value=\"<%=push_interval%>\"></td></tr><tr><td>Timezone</td><td><input name=timezone_offset size=3 id=timezone_offset value=\"<%=timezone_offset%>\"> <select name=DropDownTimezone id=DropDownTimezone onchange=\"document.getElementById('timezone_offset').value=document.getElementById('DropDownTimezone').value;\"> <option value>(Please Select)</option> <option value=-12.0>(GMT -12:00) Eniwetok, Kwajalein</option> <option value=-11.0>(GMT -11:00) Midway Island, Samoa</option> <option value=-10.0>(GMT -10:00) Hawaii</option> <option value=-9.0>(GMT -9:00) Alaska</option> <option value=-8.0>(GMT -8:00) Pacific Time (US &amp; Canada)</option> <option value=-7.0>(GMT -7:00) Mountain Time (US &amp; Canada)</option> <option value=-6.0>(GMT -6:00) Central Time (US &amp; Canada), Mexico City</option> <option value=-5.0>(GMT -5:00) Eastern Time (US &amp; Canada), Bogota, Lima</option> <option value=-4.0>(GMT -4:00) Atlantic Time (Canada), Caracas, La Paz</option> <option value=-3.5>(GMT -3:30) Newfoundland</option> <option value=-3.0>(GMT -3:00) Brazil, Buenos Aires, Georgetown</option> <option value=-2.0>(GMT -2:00) Mid-Atlantic</option> <option value=-1.0>(GMT -1:00 hour) Azores, Cape Verde Islands</option> <option value=0.0>(GMT) Western Europe Time, London, Lisbon, Casablanca</option> <option value=1.0>(GMT +1:00 hour) Brussels, Copenhagen, Madrid, Paris</option> <option value=2.0>(GMT +2:00) Kaliningrad, South Africa</option> <option value=3.0>(GMT +3:00) Baghdad, Riyadh, Moscow, St. Petersburg</option> <option value=3.5>(GMT +3:30) Tehran</option> <option value=4.0>(GMT +4:00) Abu Dhabi, Muscat, Baku, Tbilisi</option> <option value=4.5>(GMT +4:30) Kabul</option> <option value=5.0>(GMT +5:00) Ekaterinburg, Islamabad, Karachi, Tashkent</option> <option value=5.5>(GMT +5:30) Bombay, Calcutta, Madras, New Delhi</option> <option value=5.75>(GMT +5:45) Kathmandu</option> <option value=6.0>(GMT +6:00) Almaty, Dhaka, Colombo</option> <option value=7.0>(GMT +7:00) Bangkok, Hanoi, Jakarta</option> <option value=8.0>(GMT +8:00) Beijing, Perth, Singapore, Hong Kong</option> <option value=9.0>(GMT +9:00) Tokyo, Seoul, Osaka, Sapporo, Yakutsk</option> <option value=9.5>(GMT +9:30) Adelaide, Darwin</option> <option value=10.0>(GMT +10:00) Eastern Australia, Guam, Vladivostok</option> <option value=11.0>(GMT +11:00) Magadan, Solomon Islands, New Caledonia</option> <option value=12.0>(GMT +12:00) Auckland, Wellington, Fiji, Kamchatka</option></select></td></tr><tr><td>Multicast Port</td><td><input name=multicast_port size=7 value=\"<%=multicast_port%>\"></td></tr><tr><td>Serial Baudrate</td><td><input name=serial_baudrate size=7 value=\"<%=serial_baudrate%>\"></td></tr><tr><td>Debug</td><td><input type=checkbox name=debug size=10 id=debug value=debug <%=debug.checked%> /></td></tr><tr><td><input type=button value=home onclick=\"window.location='/';\"></td><td><input type=submit value=save name=do></td></tr></table></div></form></body></html>\n";

