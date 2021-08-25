# Automatic Pubkey Exchange

When this function is enabled, the key-to-content public key you used for signing will be uploaded to the public server during encryption and signature operations.
When the recipient performs decryption and verification operations, your public key will be automatically imported.

This feature is enabled by default, if you do not want to enable it, you can uncheck this feature in the advanced tab in the settings.

Disclaimer: This function is well-designed, and only when you can prove that you hold the private key of the public key, the exchange operation can proceed normally.
