# Public Key Sync

Maintaining the synchronization of your local key with the one on the key server
is of paramount importance in certain situations. Such scenarios may include the
revocation of a key by its owner, necessitating the immediate discontinuation of
its use to prevent potential man-in-the-middle attacks. Alternatively, if the
key owner generates a new signing subkey, gpg won't be able to authenticate the
validity of the encrypted text if it's signed with the subkey and you lack the
local information about the subkey.

## How to use

The utilization of this feature is straightforward. By clicking the Sync Public
Key button in the Key Management interface, the process is automated.

![image-20220109194459557](https://cdn.bktus.com/wp-content/uploads/2023/08/image-20220109194459557.png)

This functionality checks all the public keys you currently possess (only public
keys, no private keys are involved). It then seeks it on the key server. If the
corresponding public key is found on the server, GpgFrontend imports the updated
public key from the server to the local machine.

### Key Server Utilized

How do you determine which key server GpgFrontend accessed? It's quite simple.
GpgFrontend exchanges keys using the default key server that you've configured.
If you need to modify the key server you wish to access, you merely need to add
your key server in the settings and set it as the default.

![image-20220109194546570](https://cdn.bktus.com/wp-content/uploads/2023/08/image-20220109194546570.png)
