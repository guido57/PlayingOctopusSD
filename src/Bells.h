


/*
    handler to edit and save bells' configuration
*/
String postAuxBells(AutoConnectAux& aux, PageArgument& args) {
  
  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();
  Serial.printf("postAuxBells: the selected_bell is %d\r\n",selected_bell.selected);

  if(selected_bell.selected >0){
    AutoConnectText& pin = auxBells["pin"].as<AutoConnectText>();
    pin.value = String(config.Bells[selected_bell.selected-1].pin);
    AutoConnectText& note = auxBells["note"].as<AutoConnectText>();
    note.value = String(config.Bells[selected_bell.selected-1].note);
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    target.value = String(config.Bells[selected_bell.selected-1].target);
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    rest.value = String(config.Bells[selected_bell.selected-1].rest);
    AutoConnectText& target_time_ms = auxBells["target_time"].as<AutoConnectText>();
    target_time_ms.value = String(config.Bells[selected_bell.selected-1].target_time_ms);
    AutoConnectText& rest_time_ms = auxBells["rest_time"].as<AutoConnectText>();
    rest_time_ms.value = String(config.Bells[selected_bell.selected-1].rest_time_ms);
  }

  // this script allows to /bells to show the 
  // settings of the selected bell without relaoding the page 
  const char* scCopyText = R"(
    <script>
      function loadBell(bell_index) {
        const xhttp = new XMLHttpRequest();
        xhttp.onload = function() {
          bell_settings = this.responseText.split(",");
          document.getElementById("pin").value = bell_settings[0];          
          document.getElementById("note").value = bell_settings[1];          
          document.getElementById("target").value = bell_settings[2];          
          document.getElementById("rest").value = bell_settings[3];          
          document.getElementById("target_time").value = bell_settings[4];          
          document.getElementById("rest_time").value = bell_settings[5];          
        }
        xhttp.open("GET", "/download?bell=" + bell_index);
        xhttp.send();
      }
      
      // add an onchange event to select_bell 
      var select_bell_obj = document.getElementById("select_bell"); 
      select_bell_obj.addEventListener(
            'change',
            function() { 
              loadBell(select_bell_obj.selectedIndex);  
            },
            false
      );
      loadBell(select_bell_obj.selectedIndex);
       
    </script>
  )";
  AutoConnectElement& obj = aux["object"];
  obj.value = String(scCopyText);
 
  return String();
}



/*
    handler to download a bell configuration as a csv string
*/
String postAuxDownloadBell(AutoConnectAux& aux, PageArgument& args) {
  
  if(args.hasArg("bell")){
    int bell_ndx = args.arg("bell").toInt();
    
    if(bell_ndx >=0 && bell_ndx <= 5){
      String sd = String(config.Bells[bell_ndx].pin)   + "," + 
                  String(config.Bells[bell_ndx].note)   + "," + 
                  String(config.Bells[bell_ndx].target) + "," + 
                  String(config.Bells[bell_ndx].rest)   +  "," + 
                  String(config.Bells[bell_ndx].target_time_ms) + "," + 
                  String(config.Bells[bell_ndx].rest_time_ms);

      server.send(200, "text/html", sd );
    }
    else
      server.send(200, "text/html", "Please, specify a valid bell index (n=0...5)   /download?bell=n ");
  }else{
      server.send(200, "text/html", "Please, specify a valid bell index (n=0...5)   /download?bell=n ");
  }

  return String();
}


/*
    handler to save a bell by the /bells page
*/
String postAuxSaveBell(AutoConnectAux& aux, PageArgument& args) {

  // copy the values from the page /bells to the struct Bells
  Serial.printf("postAuxSaveBell:\r\n");
    
  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();
  
  if(selected_bell.selected >0){
    Serial.printf("postAuxSaveBell: the selected_bell which will be saved to disk is %d\r\n",selected_bell.selected);
    AutoConnectText& pin = auxBells["pin"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].pin = pin.value.toInt();
    AutoConnectText& note = auxBells["note"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].note = note.value.toInt();
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].target = target.value.toInt();
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].rest = rest.value.toInt();
    AutoConnectText& target_time_ms = auxBells["target_time"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].target_time_ms = target_time_ms.value.toInt();
    AutoConnectText& rest_time_ms = auxBells["rest_time"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].rest_time_ms = rest_time_ms.value.toInt();
    // Simply save to file
    writeConfigToFile("/config.json");
  }

  aux.redirect("/bells");
  return String();
}

/*
    handler to test a bell moving the mallet to the target position
*/
String postAuxTargetBell(AutoConnectAux& aux, PageArgument& args) {

  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();

  if(selected_bell.selected >0){
    Serial.printf("postAuxtargetBell: test target the selected_bell to test the target position \r\n");
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    int target_int = target.value.toInt();
    Serial.printf("postAuxtargetBell: test target %d for bell %d\r\n", target_int, selected_bell.selected-1);
    switch(selected_bell.selected){
      case 1:
      case 2: 
        mytbservo7779->servo->write(target_int);
        break;
      case 3:
      case 4: 
        mytbservo8183->servo->write(target_int);
        break;
      case 5:
      case 6: 
        mytbservo8486->servo->write(target_int);
        break;
    }
  }

  aux.redirect("/bells");
  return String();
}

/*
    handler to test a bell moving the mallet to the rest position
*/
String postAuxRestBell(AutoConnectAux& aux, PageArgument& args) {

  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();

  if(selected_bell.selected >0){
    Serial.printf("postAuxRestBell: test rest the selected_bell to test the rest position \r\n");
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    int rest_int = rest.value.toInt();
    Serial.printf("postAuxRestBell: test rest %d for bell %d\r\n", rest_int, selected_bell.selected-1);
    switch(selected_bell.selected){
      case 1:
      case 2: 
        mytbservo7779->servo->write(rest_int);
        break;
      case 3:
      case 4: 
        mytbservo8183->servo->write(rest_int);
        break;
      case 5:
      case 6: 
        mytbservo8486->servo->write(rest_int);
        break;
    }
  }

  aux.redirect("/bells");
  return String();
}

/*
    handler to hit a bell 
*/
String postAuxTestBell(AutoConnectAux& aux, PageArgument& args) {

  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();

  if(selected_bell.selected >0){
    Serial.printf("postAuxTestBell: hit the selected_bell \r\n");
    bell_struct bs;
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    bs.target = target.value.toInt();
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    bs.rest = rest.value.toInt();
    AutoConnectText& target_time = auxBells["target_time"].as<AutoConnectText>();
    bs.target_time_ms = target_time.value.toInt();
    AutoConnectText& rest_time = auxBells["rest_time"].as<AutoConnectText>();
    bs.rest_time_ms = rest_time.value.toInt();
    
    Serial.printf("postAuxtargetBell: hit bell %d\r\n", selected_bell.selected-1);
    switch(selected_bell.selected){
      case 1:
      case 2: 
        mytbservo7779->targetTo(&bs);
        break;
      case 3:
      case 4: 
        mytbservo8183->targetTo(&bs);;
        break;
      case 5:
      case 6: 
        mytbservo8486->targetTo(&bs);;
        break;
    }
  }

  aux.redirect("/bells");
  return String();
}
