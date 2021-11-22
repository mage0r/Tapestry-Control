

void mqtt_setup() {
  /* get the IP address of server by MDNS name */
  display_println(F("mDNS responder started"));
  IPAddress serverIp = MDNS.queryHost(serverHostname);
  display_print(F("IP address of server: "));
  display_println(serverIp.toString());
  /* set SSL/TLS certificate */
  //loadCA(SPIFFS, "/cert.ca");
  //espClient.setCACert(ca_cert);
  /* configure the MQTT server with IPaddress and port */
  client.setServer(serverHostname, 8883);
  //client.setServer("103.1.184.179", 8883);
  /* this receivedCallback function will be invoked 
  when client received subscribed topic */
  client.setCallback(receivedCallback);

  display_println(F("MQTT Initialised."));
}

void receivedCallback(char* topic, byte* payload, unsigned int length) {
  display_print(F("Message received: "));
  display_println(topic);

  display_print(F("payload: "));
  for (int i = 0; i < length; i++) {
    display_print(String((char)payload[i]));
  }
  display_println("");

}

void mqttconnect() {
  /* Loop until reconnected */
  while (!client.connected()) {
    display_print(F("MQTT connecting ..."));
    /* client ID */
    char clientId[] = "ESP32Client";

    client.setKeepAlive( 300 );
    /* connect now */
    if (client.connect(NAME, MQTT_USER, MQTT_PASS)) {
    //if (client.connect("arduinoClient")) {
      display_println(F("connected"));
      mqtt_connect = true;
      client.publish(MQTT_TOPIC," - Connected");
      /* subscribe topic */
      //client.subscribe(LED_TOPIC);
    } else {
      display_print(F("failed, status code ="));
      display_print(String(client.state()));
      mqtt_connect = false;
      display_println(F("try again in 5 seconds"));
      /* Wait 5 seconds before retrying */
      delay(5000); // needs to be non-blocking!
    }
    display_header();
  }
}
