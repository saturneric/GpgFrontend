# Key Server Operations

You want to use encrypted communication, but in some cases, you only know the email address to which your message is
sent, but you don't know what the public key of the owner of the email address is.

This is one situation, and another situation is that your key is accidentally leaked. How do you notify the person who
holds your public key to stop using your public key to continue sending you encrypted information? For these people, you
may not even know their names or contact information. In the above cases, you may be able to use the key server to
complete the sharing of key information. You can upload your public key information to the key server, or search or pull
the public key you need from the key server by email address and key ID.

Once the public key information is uploaded to the key server, it will be transmitted between the key servers until
finally all the key servers store your public key for access by people all over the world.

GpgFrontend provides the ability to interact with the key server. Through mouse operation, you can quickly use the key
server to share your public key, or search and import the public key you want. It should be noted that once the public
key information is uploaded to the key server, it means that the public key information cannot be deleted from the key
server and will always be retained. but when you add a subkey to your key pair, the public key of the old key pair can
be overwritten by updating.

## Import Public Key From Key Server

In the main page or in the key manager's Import key operation mode, there is a key server option. After selecting this
option you can see such an interface.

![import-keys-fomr-keyserver](_media/key-server-operations/import-keys-fomr-keyserver.png)

You can get a list of public keys associated with a key server by searching for Key ID, fingerprint or email address via
the search box. If there is a suitable public key in the list, you can import it by double-clicking it.

![import-keys-fomr-keyserver-1](_media/key-server-operations/import-keys-fomr-keyserver-1.png)

When the import is complete, you can check whether the public key is actually imported through the pop-up window (no
need to import when the local public key is newer), and you can also check some brief information about the public key.

![image-20220109191357259](_media/key-server-operations/image-20220109191357259.png)

It is worth noting that the public key you imported may be expired or revoked. You can view the status of the key
through the category tab in the key management interface. In addition to the search box, you also noticed that you can
choose which key server to grab the public key information from by clicking on the drop-down box. How to set or add this
candidate list, please refer to the last section of this document: Key server related settings.

## Export My Public Key To The Keyserver

When the current key pair has a master key, you can publish the public key information to the key server. Note that in
order to be able to let users know what they are actually doing, GpgFrontend specifies that this can only be done if a
master key exists for the local key pair. This avoids confusion about the function.

### How To Use

You can find the entry of this operation through the operation tab of the key pair detail interface, as shown in the
following figure.

![image-20220109192532368](_media/key-server-operations/image-20220109192532368.png)

Perform the operation by clicking Upload key pair to key server. Note that the naming of operations here is a bit
confusing, but this is where your public key information (not your private key) will be uploaded.

### Synchronize public key information from a key server

Sometimes, before you perform an encryption operation, you want to know if the public key you are using is still valid.
At this point, you can get the latest information about the key from the key server (if the public key server has one).

As above, you can find this action in the Actions tab of the key pair details screen, as shown in the image below.

### Extra Information

Gpg Frontend will upload the public key information to the default key server you set. The private key information is
not uploaded and should not be manually uploaded anywhere by the user.

Refer to the last section of this document on how to set the default key server.

![image-20220109192532368](_media/key-server-operations/image-20220109192532368.png)

By clicking Synchronize key pair with key server, the public key information can be automatically pulled from the key
server and compared with the local key information. After the operation is complete, you can check in the pop-up window
whether the key has actually been updated. It is worth noting that you will not be able to perform this operation if the
private key exists locally, the reason is that you already have the key pair and you should publish the latest
information for the key pair instead of accepting outdated information from the key server .

### Extra Information

Gpg Frontend will automatically communicate with the default keyserver you set and get the information it wants. Refer
to the last section of this document on how to set the default key server.

## Sync ALL Public Key

This is an advanced function provided by Gpg Frontend, it can synchronize all your local public key information at one
time, if you want to know, please read [this document](../features/sync-all-public-keys.md).

## Key Server Related Settings

如何What about setting a list of keyservers? Or set a default keyserver? At this point, you need to open the Settings
interface and find the Key Servers tab. Here you can see operations related to the key server candidate list, and see
which key server is the default key server.

![image-20220109195518834](_media/key-server-operations/image-20220109195518834.png)

You can enter the http or https address of the key server you want to add in the input box, and then click Add to add a
candidate key server. In order to prevent man-in-the-middle attacks, users are strongly recommended to use the https
protocol. If you want to delete a candidate key server, you can right-click the row of the corresponding key server in
the table and click Delete in the pop-up menu. If you want to edit an existing candidate key server address, you can
double-click its address in the table and edit it.

If you want to test the network connectivity of the servers in the key server candidate list, you can click the Test
button at the bottom. Note that the test here only tells you if the keyserver is reachable, not whether the address is a
valid keyserver.

### Set Default Key Server

If you want to set a candidate key server as your default key server, you can right-click the row of the corresponding
key server in the table, and click Set as Default in the pop-up menu. You can see if a candidate keyserver is the
default keyserver in the first column of the table.