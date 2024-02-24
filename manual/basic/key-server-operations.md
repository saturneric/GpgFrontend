# Key Server Operations

Key servers play a pivotal role in the ecosystem of encrypted communication,
serving as a centralized repository for public key information. These servers
enable individuals to share and retrieve public keys necessary for encrypted
messaging, even when direct exchange is not feasible. Key servers are
particularly useful in scenarios where secure communication needs to be
established without prior direct contact, or when a user's public key needs to
be widely distributed or updated due to security concerns.

When you wish to send an encrypted message but lack the recipient's public key,
key servers offer a solution by allowing you to search for and retrieve the
public key associated with the recipient's email address or key ID. This process
facilitates the encryption of messages in a way that ensures only the intended
recipient, who possesses the corresponding private key, can decrypt and read the
message.

Moreover, key servers are integral to maintaining the integrity and
trustworthiness of the public key infrastructure. If a user's private key is
compromised, it is crucial to inform others not to use the associated public key
for encrypting messages anymore. By uploading a new public key to a key server
and marking the old one as obsolete or compromised, users can mitigate the risks
associated with the exposure of their private key.

The functionality of key servers is enhanced by software tools such as
GpgFrontend, which simplifies the process of managing public keys. With
GpgFrontend, users can effortlessly upload their public key to key servers,
search for other users' public keys using an email address or key ID, and import
these keys for use in encrypted communication. The software's user-friendly
interface enables these operations to be performed with just a few mouse clicks,
making encrypted communication more accessible to a broader audience.

It is important to note that once public key information is uploaded to a key
server, it is propagated across a network of key servers worldwide, making it
available to anyone who searches for it. This wide distribution ensures that
encrypted communication can be established easily across different platforms and
geographical locations. However, users should be aware that public keys uploaded
to key servers cannot be deleted, emphasizing the importance of careful key
management. In situations where a key needs to be updated, such as when adding a
subkey to a key pair, the new key information can overwrite the old one on the
server, thus maintaining the security and relevance of the key information
available to the public.

In summary, key servers are essential for the secure and efficient exchange of
encrypted messages, offering a reliable method for sharing and retrieving public
keys. They support the integrity of secure communications by facilitating the
widespread distribution of public keys and enabling users to update or replace
keys when necessary.

## Import Public Key From Key Server

In the main page or in the key manager's Import key operation mode, there is a
key server option. After selecting this option you can see such an interface.

![Import Public Key From Key Server](https://image.cdn.bktus.com/i/2023/11/16/d75cb252-9a65-5b73-01cd-a45b5ff501ef.webp)

You can get a list of public keys associated with a key server by searching for
Key ID, fingerprint or email address via the search box. If there is a suitable
public key in the list, you can import it by double-clicking it.

![Import Public Key From Key Server 1](https://image.cdn.bktus.com/i/2023/11/16/ae422544-3764-0fe0-638a-d731715acf3e.webp)

When the import is complete, you can check whether the public key is actually
imported through the pop-up window (no need to import when the local public key
is newer), and you can also check some brief information about the public key.

![Import Public Key From Key Server 2](https://image.cdn.bktus.com/i/2023/11/16/cbb78f5f-3620-1534-4b4e-e7752e1c9aa4.webp)

It is important to note that the public key you import may have expired or been
revoked. You can check the status of the key by navigating to the category tab
in the key management interface. In addition to the search box, you may also
notice a drop-down box that allows you to choose which key server to retrieve
the public key information from. To modify or add to this list of candidate
servers, please refer to the last section of this document: Key server related
settings.

## Export My Public Key To The Key Server

If the current key pair has a master key, you have the option to publish the
public key information to a key server. It is important to note that in order to
avoid confusion, GpgFrontend requires the presence of a master key for this
action to be performed. This ensures that users are aware of what they are doing
and the function being performed.

### How To Use

You can find the entry of this operation through the operation tab of the key
pair detail interface, as shown in the following figure.

![Export My Public Key To The Key Server](https://image.cdn.bktus.com/i/2023/11/16/87b435b1-3eb2-421d-c8cb-f6d926b6a1c7.webp)

Perform the operation by clicking Upload key pair to key server. Note that the
naming of operations here is a bit confusing, but this is where your public key
information (not your private key) will be uploaded.

### Synchronize public key information from a key server

Sometimes, before you perform an encryption operation, you want to know if the
public key you are using is still valid. At this point, you can get the latest
information about the key from the key server (if the public key server has
one).

As above, you can find this action in the Actions tab of the key pair details
screen, as shown in the image below.

GpgFrontend will upload the public key information to the default key server
you set. The private key information is not uploaded and should not be manually
uploaded anywhere by the user.

Refer to the last section of this document on how to set the default key server.

![Set Default Key Server](https://image.cdn.bktus.com/i/2023/11/16/87b435b1-3eb2-421d-c8cb-f6d926b6a1c7.webp)

The "Synchronize key pair with key server" function allows for automatic
retrieval of public key information from the key server, which is then compared
with the local key information. After the operation is completed, a pop-up
window will appear indicating whether the key has actually been updated. It
should be noted that this operation is not possible if the private key exists
locally. This is because, in such a case, you already have the key pair and
should publish the latest information for the key pair instead of accepting
outdated information from the key server.

### Extra Information

GpgFrontend automatically communicates with the default key server that you have
set to obtain the necessary information. You can refer to the last section of
this document to learn how to set the default key server.

## Sync ALL Public Key

This is an advanced function provided by GpgFrontend, it can synchronize all
your local public key information at one time, if you want to know, please read
[this document](../features/sync-all-public-keys.md).

## Key Server Related Settings

If you want to set a list of key servers or a default key server, you can do so
by accessing the Settings interface and navigating to the Key Servers tab. Here,
you will find options for managing your key server candidate list and
determining which key server is set as the default.

![Key Server Related Settings](https://image.cdn.bktus.com/i/2023/11/16/afe69b9b-0576-d275-91df-79585c245b22.webp)

To add a candidate key server to the list, simply enter the http or https
address of the key server you wish to add into the input box and click "Add". It
is strongly recommended that users use the https protocol to prevent
man-in-the-middle attacks. If you wish to delete a candidate key server, simply
right-click on the corresponding row in the table and select "Delete" from the
pop-up menu. To edit an existing candidate key server address, double-click on
the address in the table and edit it.

To test the network connectivity of the servers in the key server candidate
list, click the "Test" button located at the bottom of the Key Servers tab.
However, note that the test only determines if the keyserver is reachable, not
whether the address is a valid keyserver.

### Set Default Key Server

To set a candidate key server as your default key server, you can follow these
steps. First, locate the candidate key server you want to set as the default in
the table. Then, right-click the row of the corresponding key server, and click
"Set as Default" in the pop-up menu. Once set, you can verify whether a
candidate key server is the default key server by checking the first column of
the table.
