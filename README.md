# basicesp32toAwsthroughMQTT
esp32 sends data to aws every  certification
 Device---(MQTT Broker)AWS IoT core.

 1.By the device side
   .Configure wifi
   .Configure certifaction to communicate with AWS core
   .Define the same THingname as AWS IoT core(thingsname)
   .Define topic(both subscript and pubscript) 

  2. AWS IoT core
     https://eu-central-1.console.aws.amazon.com/iot/home?region=eu-central-1#/test

     Create single thing- attach/create policy----create certification.
     There are four policy actions: iot:connect,iot:publish,iot:subscribe and iot:Receive.
     Policy resource are all "*"

3. Run the code of device side and check the delivered data on the AWS IoT core side

   Test--MQTT test client 
   subscrib/publish to a topic, to check if it works well.


     
  

   

   
