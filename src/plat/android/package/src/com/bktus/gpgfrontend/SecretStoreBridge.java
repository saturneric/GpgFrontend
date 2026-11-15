// SPDX-License-Identifier: GPL-3.0-or-later

package com.bktus.gpgfrontend;

import android.app.KeyguardManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyPermanentlyInvalidatedException;
import android.security.keystore.KeyProperties;
import android.security.keystore.StrongBoxUnavailableException;
import android.security.keystore.UserNotAuthenticatedException;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.Key;
import java.security.KeyStore;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

import javax.crypto.AEADBadTagException;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;

/**
 * The credential store behind src/plat/android/AndroidSecretStore.cpp.
 *
 * The Android Keystore holds keys, not arbitrary secrets, so each entry is
 * two things: a non-exportable AES-256-GCM key in the Keystore, and the
 * secret sealed with it in a file under getNoBackupFilesDir(). Neither is
 * of any use without the other, and the key never leaves the device's
 * secure hardware.
 *
 * Every call answers with one status byte followed by its payload: the
 * secret for a read that worked, otherwise the reason in UTF-8. What the
 * byte says decides whether the user is ever offered a reset, so a failure
 * that may be temporary -- the device is locked, the Keystore is not
 * answering -- must never read as ABSENT or CORRUPT.
 */
public final class SecretStoreBridge {
    // Must match SecretReadStatus's order in AndroidSecretStore.cpp.
    static final byte OK = 0;
    static final byte ABSENT = 1;
    static final byte CORRUPT = 2;
    static final byte LOCKED = 3;
    static final byte UNAVAILABLE = 4;

    private static final String TAG = "GpgFrontend";
    private static final String KEYSTORE = "AndroidKeyStore";
    private static final String TRANSFORMATION = "AES/GCM/NoPadding";
    private static final String DIRECTORY = "secret_store";
    private static final byte FORMAT_VERSION = 1;
    private static final int IV_LENGTH = 12;
    private static final int TAG_BITS = 128;

    /**
     * Android 15, where unlocked-device-required keys stopped being deleted
     * with the secure lock screen and became usable without one
     * (KeyGenParameterSpec.Builder#setUnlockedDeviceRequired). Below it the
     * key is bound to the hardware only.
     */
    private static final int UNLOCKED_DEVICE_REQUIRED_SDK = 35;

    private static final Object LOCK = new Object();

    private SecretStoreBridge() {}

    /** The secret of one entry. */
    public static byte[] read(Context context, String service, String account) {
        synchronized (LOCK) {
            final String name = entryName(service, account);
            byte[] blob = null;
            byte[] plain = null;
            try {
                blob = readBlob(context, name);
                final Key key = keyStore().getKey(alias(name), null);
                // Either half alone opens nothing: the entry is gone.
                if (blob == null || !(key instanceof SecretKey)) {
                    return answer(ABSENT, blob == null ? "no sealed secret"
                                                       : "no keystore key");
                }
                if (blob.length < 1 + IV_LENGTH + TAG_BITS / 8
                        || blob[0] != FORMAT_VERSION) {
                    return answer(CORRUPT, "sealed secret is malformed");
                }
                final Cipher cipher = Cipher.getInstance(TRANSFORMATION);
                cipher.init(Cipher.DECRYPT_MODE, key,
                        new GCMParameterSpec(TAG_BITS, blob, 1, IV_LENGTH));
                cipher.updateAAD(associatedData(service, account));
                plain = cipher.doFinal(blob, 1 + IV_LENGTH, blob.length - 1 - IV_LENGTH);

                final byte[] out = new byte[1 + plain.length];
                out[0] = OK;
                System.arraycopy(plain, 0, out, 1, plain.length);
                return out;
            } catch (Throwable e) {
                return answer(classify(context, e), describe(e));
            } finally {
                if (plain != null) Arrays.fill(plain, (byte) 0);
                if (blob != null) Arrays.fill(blob, (byte) 0);
            }
        }
    }

    /** Seal @p secret into the entry, replacing what it held. */
    public static byte[] write(Context context, String service, String account,
                               byte[] secret) {
        synchronized (LOCK) {
            final String name = entryName(service, account);
            byte[] ciphertext = null;
            byte[] blob = null;
            try {
                final KeyStore store = keyStore();
                Key key = store.getKey(alias(name), null);
                if (!(key instanceof SecretKey)) key = createKey(context, alias(name));

                final Cipher cipher = Cipher.getInstance(TRANSFORMATION);
                try {
                    cipher.init(Cipher.ENCRYPT_MODE, key);
                } catch (KeyPermanentlyInvalidatedException e) {
                    // What it sealed is unreadable anyway, and this write
                    // replaces it: start the entry over with a fresh key.
                    store.deleteEntry(alias(name));
                    key = createKey(context, alias(name));
                    cipher.init(Cipher.ENCRYPT_MODE, key);
                }
                cipher.updateAAD(associatedData(service, account));
                ciphertext = cipher.doFinal(secret);
                final byte[] iv = cipher.getIV();
                if (iv == null || iv.length != IV_LENGTH) {
                    return answer(UNAVAILABLE, "unexpected GCM nonce length");
                }

                blob = new byte[1 + IV_LENGTH + ciphertext.length];
                blob[0] = FORMAT_VERSION;
                System.arraycopy(iv, 0, blob, 1, IV_LENGTH);
                System.arraycopy(ciphertext, 0, blob, 1 + IV_LENGTH, ciphertext.length);
                writeBlob(context, name, blob);
                return answer(OK, "");
            } catch (Throwable e) {
                return answer(classify(context, e), describe(e));
            } finally {
                if (ciphertext != null) Arrays.fill(ciphertext, (byte) 0);
                if (blob != null) Arrays.fill(blob, (byte) 0);
            }
        }
    }

    /** Delete both halves of the entry. OK when they are gone, as they may already be. */
    public static byte[] remove(Context context, String service, String account) {
        synchronized (LOCK) {
            final String name = entryName(service, account);
            try {
                final File blob = blobFile(context, name);
                if (blob.exists() && !blob.delete()) {
                    return answer(UNAVAILABLE, "could not delete the sealed secret");
                }
                final KeyStore store = keyStore();
                if (store.containsAlias(alias(name))) store.deleteEntry(alias(name));
                return answer(OK, "");
            } catch (Throwable e) {
                return answer(classify(context, e), describe(e));
            }
        }
    }

    private static KeyStore keyStore() throws Exception {
        final KeyStore store = KeyStore.getInstance(KEYSTORE);
        store.load(null);
        return store;
    }

    /**
     * A new key, in StrongBox where the device has one and in the TEE
     * otherwise; bound to an unlocked device where that is safe to ask for.
     */
    private static SecretKey createKey(Context context, String alias) throws Exception {
        final boolean strongBox = Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
                && context.getPackageManager()
                          .hasSystemFeature(PackageManager.FEATURE_STRONGBOX_KEYSTORE);
        final boolean unlocked = Build.VERSION.SDK_INT >= UNLOCKED_DEVICE_REQUIRED_SDK;

        Exception last = null;
        // Strongest first. Every fallback keeps the key in secure hardware.
        final boolean[][] attempts = {
            {strongBox, unlocked}, {false, unlocked}, {false, false},
        };
        for (final boolean[] attempt : attempts) {
            if (attempt[0] && !strongBox) continue;
            try {
                final SecretKey key = generate(alias, attempt[0], attempt[1]);
                Log.i(TAG, "secret store key created, strongbox: " + attempt[0]
                        + ", unlocked device required: " + attempt[1]);
                return key;
            } catch (StrongBoxUnavailableException e) {
                last = e;
            } catch (Exception e) {
                // A locked device fails every attempt the same way; say so
                // rather than trying weaker ones.
                if (isDeviceLocked(context)) throw e;
                last = e;
            }
        }
        throw last != null ? last : new IllegalStateException("no key generated");
    }

    private static SecretKey generate(String alias, boolean strongBox, boolean unlocked)
            throws Exception {
        final KeyGenParameterSpec.Builder spec = new KeyGenParameterSpec.Builder(alias,
                KeyProperties.PURPOSE_ENCRYPT | KeyProperties.PURPOSE_DECRYPT)
                .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                .setKeySize(256)
                .setRandomizedEncryptionRequired(true);
        if (strongBox && Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            spec.setIsStrongBoxBacked(true);
        }
        if (unlocked && Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            spec.setUnlockedDeviceRequired(true);
        }
        final KeyGenerator generator =
                KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, KEYSTORE);
        generator.init(spec.build());
        return generator.generateKey();
    }

    /**
     * Which status a failure is. In doubt it is never ABSENT or CORRUPT:
     * those lead to the offer of a reset, which destroys the profile.
     */
    private static byte classify(Context context, Throwable e) {
        // Gone for good: the key can never open what it sealed again.
        if (hasCause(e, KeyPermanentlyInvalidatedException.class)) return ABSENT;

        // Anything while the device is locked is the lock's doing, and
        // unlocking ends it.
        if (isDeviceLocked(context)) return LOCKED;
        if (hasCause(e, UserNotAuthenticatedException.class)) return LOCKED;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && requiresUserAuthentication(e)) {
            return LOCKED;
        }

        // The tag did not match with the device unlocked: the sealed secret
        // or its key is not what was written.
        if (hasCause(e, AEADBadTagException.class)) return CORRUPT;

        return UNAVAILABLE;
    }

    /** API 33: KeyStoreException says outright whether a lock is the cause. */
    private static boolean requiresUserAuthentication(Throwable e) {
        for (Throwable t = e; t != null; t = t.getCause()) {
            if (t instanceof android.security.KeyStoreException
                    && ((android.security.KeyStoreException) t).requiresUserAuthentication()) {
                return true;
            }
        }
        return false;
    }

    private static boolean hasCause(Throwable e, Class<? extends Throwable> type) {
        for (Throwable t = e; t != null; t = t.getCause()) {
            if (type.isInstance(t)) return true;
        }
        return false;
    }

    private static boolean isDeviceLocked(Context context) {
        try {
            final KeyguardManager keyguard = context.getSystemService(KeyguardManager.class);
            return keyguard != null && keyguard.isDeviceLocked();
        } catch (RuntimeException e) {
            return false;
        }
    }

    private static String describe(Throwable e) {
        final String message = e.getMessage();
        return e.getClass().getSimpleName() + (message != null ? ": " + message : "");
    }

    private static byte[] answer(byte status, String detail) {
        // The reason only, never data: what a bug report about a profile that
        // will not open needs, and nothing the secret could be read from.
        if (status != OK) Log.w(TAG, "secret store: " + statusName(status) + ": " + detail);
        final byte[] text = detail.getBytes(StandardCharsets.UTF_8);
        final byte[] out = new byte[1 + text.length];
        out[0] = status;
        System.arraycopy(text, 0, out, 1, text.length);
        return out;
    }

    private static String statusName(byte status) {
        switch (status) {
            case ABSENT: return "absent";
            case CORRUPT: return "corrupt";
            case LOCKED: return "locked";
            default: return "unavailable";
        }
    }

    /** One name per entry, safe as a file name and a key alias. */
    private static String entryName(String service, String account) {
        final byte[] digest;
        try {
            digest = MessageDigest.getInstance("SHA-256")
                    .digest(associatedData(service, account));
        } catch (NoSuchAlgorithmException e) {
            // Every Android release has SHA-256; without it nothing here works.
            throw new IllegalStateException(e);
        }
        final StringBuilder hex = new StringBuilder(digest.length * 2);
        for (final byte b : digest) hex.append(String.format("%02x", b));
        return hex.toString();
    }

    private static String alias(String name) {
        return "gf." + name;
    }

    /** Binds a sealed secret to its entry, so one cannot be passed off as another. */
    private static byte[] associatedData(String service, String account) {
        return (service + '\0' + account).getBytes(StandardCharsets.UTF_8);
    }

    private static File blobFile(Context context, String name) {
        return new File(new File(context.getNoBackupFilesDir(), DIRECTORY), name);
    }

    private static byte[] readBlob(Context context, String name) throws IOException {
        final File file = blobFile(context, name);
        if (!file.isFile()) return null;
        final long length = file.length();
        if (length > 64 * 1024) throw new IOException("sealed secret is too large");
        final byte[] bytes = new byte[(int) length];
        try (FileInputStream in = new FileInputStream(file)) {
            int read = 0;
            while (read < bytes.length) {
                final int n = in.read(bytes, read, bytes.length - read);
                if (n < 0) throw new IOException("sealed secret was truncated");
                read += n;
            }
        }
        return bytes;
    }

    /** Replace the blob in one step: written beside it, synced, renamed over it. */
    private static void writeBlob(Context context, String name, byte[] blob)
            throws IOException {
        final File target = blobFile(context, name);
        final File directory = target.getParentFile();
        if (directory != null && !directory.isDirectory() && !directory.mkdirs()) {
            throw new IOException("cannot create " + DIRECTORY);
        }
        final File temporary = new File(directory, name + ".tmp");
        try (FileOutputStream out = new FileOutputStream(temporary)) {
            out.write(blob);
            out.getFD().sync();
        }
        if (!temporary.renameTo(target)) {
            temporary.delete();
            throw new IOException("cannot replace the sealed secret");
        }
    }
}
