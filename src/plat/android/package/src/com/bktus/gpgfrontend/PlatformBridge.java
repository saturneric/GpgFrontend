// SPDX-License-Identifier: GPL-3.0-or-later

package com.bktus.gpgfrontend;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Build;
import android.os.PersistableBundle;
import android.view.WindowManager;

import androidx.core.content.FileProvider;

import java.io.File;

/**
 * What src/plat/android/ needs from the Android framework that is
 * simpler to say in Java: the share sheet with its "target chosen" callback,
 * files shared through a FileProvider, a clipboard copy marked sensitive, and
 * FLAG_SECURE.
 */
public final class PlatformBridge {
    private static final String ACTION_TARGET_CHOSEN =
            "com.bktus.gpgfrontend.SHARE_TARGET_CHOSEN";
    private static final String EXTRA_REQUEST = "com.bktus.gpgfrontend.SHARE_REQUEST";
    /** The FileProvider of AndroidManifest.xml, serving <cache>/share only. */
    private static final String SHARE_AUTHORITY_SUFFIX = ".share";

    private static BroadcastReceiver chosenReceiver;

    private PlatformBridge() {}

    /** Reported to ShareSession::TargetChosen(). */
    private static native void onShareTargetChosen(int request);

    /**
     * Show the share sheet for text. The chooser reports the chosen target
     * through a broadcast only this application can receive.
     */
    public static boolean shareText(Activity activity, String text, String title,
                                    int request) {
        Intent send = new Intent(Intent.ACTION_SEND);
        send.setType("text/plain");
        send.putExtra(Intent.EXTRA_TEXT, text);
        return showChooser(activity, send, title, request);
    }

    /**
     * Show the share sheet for one file under <cache>/share. The receiver is
     * granted read access to that file only, until revokeFile().
     */
    public static boolean shareFile(Activity activity, String path, String mime,
                                    String title, int request) {
        try {
            Uri uri = uriFor(activity, path);
            Intent send = new Intent(Intent.ACTION_SEND);
            send.setType(mime);
            send.putExtra(Intent.EXTRA_STREAM, uri);
            // ClipData carries the grant through the chooser to the target.
            send.setClipData(ClipData.newRawUri("", uri));
            send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            return showChooser(activity, send, title, request);
        } catch (RuntimeException e) {
            return false;
        }
    }

    /** Withdraw every grant to read the file at path. */
    public static void revokeFile(Context context, String path) {
        try {
            context.revokeUriPermission(uriFor(context, path),
                                        Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (RuntimeException e) {
            // Not a file the provider serves: nothing was granted.
        }
    }

    private static Uri uriFor(Context context, String path) {
        return FileProvider.getUriForFile(
                context, context.getPackageName() + SHARE_AUTHORITY_SUFFIX,
                new File(path));
    }

    private static boolean showChooser(Activity activity, Intent send,
                                       String title, int request) {
        try {
            registerChosenReceiver(activity.getApplicationContext());

            // Explicit to this package, and mutable so the chooser can add the
            // chosen component to it.
            Intent chosen = new Intent(ACTION_TARGET_CHOSEN)
                    .setPackage(activity.getPackageName())
                    .putExtra(EXTRA_REQUEST, request);
            int flags = PendingIntent.FLAG_UPDATE_CURRENT;
            if (Build.VERSION.SDK_INT >= 31) flags |= PendingIntent.FLAG_MUTABLE;
            PendingIntent callback =
                    PendingIntent.getBroadcast(activity, request, chosen, flags);

            Intent chooser =
                    Intent.createChooser(send, title, callback.getIntentSender());
            chooser.addFlags(send.getFlags() & Intent.FLAG_GRANT_READ_URI_PERMISSION);
            activity.startActivity(chooser);
            return true;
        } catch (RuntimeException e) {
            return false;
        }
    }

    private static synchronized void registerChosenReceiver(Context context) {
        if (chosenReceiver != null) return;
        chosenReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context c, Intent intent) {
                onShareTargetChosen(intent.getIntExtra(EXTRA_REQUEST, 0));
            }
        };
        IntentFilter filter = new IntentFilter(ACTION_TARGET_CHOSEN);
        if (Build.VERSION.SDK_INT >= 33) {
            context.registerReceiver(chosenReceiver, filter,
                                     Context.RECEIVER_NOT_EXPORTED);
        } else {
            context.registerReceiver(chosenReceiver, filter);
        }
    }

    /** Copy text the system should treat as sensitive (Android 13 and later). */
    public static void setSensitiveClip(Context context, String text) {
        ClipboardManager clipboard =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("GpgFrontend", text);
        if (Build.VERSION.SDK_INT >= 33) {
            PersistableBundle extras = new PersistableBundle();
            extras.putBoolean(ClipDescription.EXTRA_IS_SENSITIVE, true);
            clip.getDescription().setExtras(extras);
        }
        clipboard.setPrimaryClip(clip);
    }

    /** Keep the activity's window out of screenshots and the app switcher. */
    public static void setSecure(Activity activity, boolean secure) {
        activity.runOnUiThread(() -> {
            if (secure) {
                activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
            } else {
                activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
            }
        });
    }
}
