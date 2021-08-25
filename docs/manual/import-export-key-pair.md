# Import & Export Key Pair

GpgFrontend provides multiple ways for users to import or export key paris. Read the guide.

# Import Key Pair

You can find the import options in the toolbar. You can choose several import methods according to your actual situation.
Let’s introduce these methods.

## File

When you select this option, you need to select a public key or private key file that meets the standard.
The file you choose should be in text format, with any extension.

## Editor

You can paste the contents of the key file on a blank text label page. Then click this option, GpgFrontend will automatically recognize and import the corresponding key.

## Clipboard

You can copy the contents of the key to your system clipboard somewhere else. Then, after selecting this option, the corresponding key will be imported.

## Keyserver

After selecting this option, you can enter the email or ID corresponding to the key in the pop-up selection, and then select the appropriate key server.
Then after clicking the search button, GpgFrontend will list the public keys that can be imported in the key server.
You can import them together, or you can choose to double-click the table row to import the corresponding public key.
Note: The keys imported in this way are all public keys.

## Dropdown on Key Toolbox

You can drag the key file directly to the key toolbox, and then follow the prompts of GpgFrontend to import the key.

# Export Key Pair

Similar to import, there are multiple export methods.

## Append Pubkey to Editor

Right-click a row in the key toolbox, and click Append Select Key(s) to Editor in the pop-up menu item. 
You will find that the public key that the key is right appears at the end of your text label page.
You can freely copy the content to any place.

##  Export Private Key

You can find private key options everywhere in the details page that contains the private key (master key or subkey). 
Then you can select a location, and GpgFrontend will export the corresponding private key content to that location later.
Generally speaking, the private key content will be bundled with the public key content to export and export.
Please note: the private key file can never be disclosed to others. If it is leaked, it means that all ciphertexts encrypted by the key are no longer safe.

