function getXmlHttp()
{
    var xmlhttp;
    if (window.XMLHttpRequest) {
        xmlhttp = new XMLHttpRequest();
    } else if (window.ActiveXObject) {
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    } else {
        alert("Your browser does not support XMLHTTP!");
    }
    return xmlhttp;
}

function handleResponse(response)
{
    var out = "<table style=\"font-size: 26px;\" cellpadding=\"2\" cellspacing=\"0\">";
    for(var key in response)
    {
        var display_value = response[key];
        if (!key.startsWith("_"))
        {
            out += "<tr><td style=\"background-color: white;\">" + key + "</td><td style=\"padding-left: 14px; text-align: right;\">" + display_value + "</td></tr>";
        }
    }
    out += "</table>";
    document.getElementById('table_display').innerHTML = out;
    document.getElementById('clock').innerHTML = response._time;
}

function dataRefresh()
{
    xmlhttp = getXmlHttp();
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState == 4)
        {
            var rtext = xmlhttp.responseText;
            if (rtext != '')
            {
                var response = eval('(' + rtext + ')');
                handleResponse(response);
            }
            setTimeout("dataRefresh()", 3000);
        }
    };
    xmlhttp.open("POST", "api.json?rnd=" + Math.random(10000), true);
    xmlhttp.send(null);
}
