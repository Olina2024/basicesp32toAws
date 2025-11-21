#add device shadow
ThingsName change to device macaddress.
1. Create an IoT Thing

Go to AWS Console → AWS IoT Core

In the left menu, go to Manage → All Devices → Things

Click Create thing

Choose Create single thing

Enter a Thing name

You can use anything, because your code overrides it with the MAC address.

Click Next

2. Create Keys & Certificates

Choose Auto-generate a new certificate

Click Create certificate

IMPORTANT → Download all four items:

Device certificate (xxxxx-certificate.pem.crt)

Private key (xxxxx-private.pem.key)

Public key (optional)

Amazon Root CA 1
(If not shown, download from: https://www.amazontrust.com/repository/AmazonRootCA1.pem
)

Activate the certificate

Click Activate

3. Attach an IoT Policy to the Certificate

AWS IoT will not allow any connection unless a policy is attached.

Create Policy:

Go to Secure → Policies

Create Policy

Use the following policy (allows publish/subscribe for your device):

{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Connect",
        "iot:Publish",
        "iot:Subscribe",
        "iot:Receive"
      ],
      "Resource": [
        "*"
      ]
    }
  ]
}
