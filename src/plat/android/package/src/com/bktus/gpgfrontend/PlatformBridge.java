// SPDX-License-Identifier: GPL-3.0-or-later

package com.bktus.gpgfrontend;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.UriPermission;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.PersistableBundle;
import android.provider.DocumentsContract;
import android.view.WindowManager;

import androidx.core.content.FileProvider;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;

/**
 * What src/plat/android/ needs from the Android framework that is
 * simpler to say in Java: the share sheet with its "target chosen" callback,
 * files shared through a FileProvider, what other applications share with
 * GpgFrontend, a clipboard copy marked sensitive, and FLAG_SECURE.
 */
public final class PlatformBridge {
    private static final String ACTION_TARGET_CHOSEN =
            "com.bktus.gpgfrontend.SHARE_TARGET_CHOSEN";
    private static final String EXTRA_REQUEST = "com.bktus.gpgfrontend.SHARE_REQUEST";
    /**
     * Held only by applications signed as GpgFrontend (AndroidManifest.xml):
     * before Android 13 a receiver registered at run time is open to every
     * application, which could otherwise say a share went through.
     */
    private static final String PERMISSION_TARGET_CHOSEN_SUFFIX =
            ".permission.SHARE_TARGET_CHOSEN";
    /** The FileProvider of AndroidManifest.xml, serving <cache>/share only. */
    private static final String SHARE_AUTHORITY_SUFFIX = ".share";

    private static BroadcastReceiver chosenReceiver;

    /** Shared with GpgFrontend and not taken yet (TakeSharedItems()). */
    private static final List<String> sharedUris = new ArrayList<>();
    private static String sharedText;

    /** The request the document picker is started with (pickDocuments()). */
    static final int REQUEST_PICK_DOCUMENTS = 0x4746;

    private PlatformBridge() {}

    /** Reported to PickDocuments()'s callback; none when the picker was left. */
    private static native void onDocumentsPicked(String[] uris);

    /**
     * Show the system's picker for documents to read. What is picked comes
     * back to GpgFrontendActivity.onActivityResult(), which hands it to
     * documentsPicked(). Any provider's documents, a cloud folder's too;
     * readable until the application ends, nothing persisted.
     */
    public static boolean pickDocuments(Activity activity, boolean multiple) {
        Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        pick.addCategory(Intent.CATEGORY_OPENABLE);
        pick.setType("*/*");
        pick.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, multiple);
        try {
            activity.startActivityForResult(pick, REQUEST_PICK_DOCUMENTS);
            return true;
        } catch (RuntimeException e) {
            return false;
        }
    }

    /** The picker's answer: one document as the data, several as clip data. */
    static void documentsPicked(int resultCode, Intent data) {
        List<String> uris = new ArrayList<>();
        if (resultCode == Activity.RESULT_OK && data != null) {
            ClipData clip = data.getClipData();
            if (clip != null) {
                for (int i = 0; i < clip.getItemCount(); ++i) {
                    Uri uri = clip.getItemAt(i).getUri();
                    if (uri != null) uris.add(uri.toString());
                }
            } else if (data.getData() != null) {
                uris.add(data.getData().toString());
            }
        }
        try {
            onDocumentsPicked(uris.toArray(new String[0]));
        } catch (UnsatisfiedLinkError notYet) {
            // The process was ended under the picker and started again:
            // nobody is waiting for this answer any more.
        }
    }

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

    /**
     * Open one file under <cache>/share in an application the person picks,
     * granted read access as shareFile() grants it.
     */
    public static boolean openFile(Activity activity, String path, String mime,
                                   String title, int request) {
        try {
            Uri uri = uriFor(activity, path);
            Intent view = new Intent(Intent.ACTION_VIEW);
            view.setDataAndType(uri, mime);
            view.setClipData(ClipData.newRawUri("", uri));
            view.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            return showChooser(activity, view, title, request);
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
            // GpgFrontend takes shares too; offering it its own makes no sense.
            chooser.putExtra(Intent.EXTRA_EXCLUDE_COMPONENTS, new ComponentName[] {
                    new ComponentName(activity, activity.getClass())});
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
        String permission = context.getPackageName() + PERMISSION_TARGET_CHOSEN_SUFFIX;
        if (Build.VERSION.SDK_INT >= 33) {
            context.registerReceiver(chosenReceiver, filter, permission, null,
                                     Context.RECEIVER_NOT_EXPORTED);
        } else {
            context.registerReceiver(chosenReceiver, filter, permission, null);
        }
    }

    /** Copy text the system should treat as sensitive (Android 13 and later). */
    public static boolean setSensitiveClip(Context context, String text) {
        try {
            ClipboardManager clipboard =
                    (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
            ClipData clip = ClipData.newPlainText("GpgFrontend", text);
            if (Build.VERSION.SDK_INT >= 33) {
                PersistableBundle extras = new PersistableBundle();
                extras.putBoolean(ClipDescription.EXTRA_IS_SENSITIVE, true);
                clip.getDescription().setExtras(extras);
            }
            clipboard.setPrimaryClip(clip);
            return true;
        } catch (RuntimeException e) {
            // Too large for the clipboard's transaction, say: not copied.
            return false;
        }
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

    /**
     * Keep what @p intent shares: the documents of ACTION_SEND and
     * ACTION_SEND_MULTIPLE, the text of an ACTION_SEND without one, the
     * document of ACTION_VIEW. Called by GpgFrontendActivity for each intent
     * it is started or resumed with; anything else is no share.
     */
    /**
     * Keep @p uri unless it names one of GpgFrontend's own providers: shared
     * by another application, it would have GpgFrontend read its own private
     * files under its own identity.
     */
    private static void keep(Context context, Uri uri) {
        if (isOwn(context, uri)) return;
        sharedUris.add(uri.toString());
    }

    /**
     * Whether uri names one of GpgFrontend's own providers. An authority may
     * carry the user it is of ("0@authority"), which the content resolver
     * drops: the provider is the one after it.
     */
    static boolean isOwn(Context context, Uri uri) {
        String authority = uri.getAuthority();
        if (authority == null) return false;
        String provider = authority.substring(authority.lastIndexOf('@') + 1);
        String own = context.getPackageName();
        return provider.equals(own) || provider.startsWith(own + ".");
    }

    /** Tells the application a share arrived; nothing before it is up. */
    private static native void onShared();

    static void announceShare() {
        try {
            onShared();
        } catch (UnsatisfiedLinkError notYet) {
            // Not loaded or not listening yet: it takes the share when its
            // page comes up.
        }
    }

    static synchronized void receive(Context context, Intent intent) {
        if (intent == null || intent.getAction() == null) return;
        switch (intent.getAction()) {
            case Intent.ACTION_SEND: {
                Uri uri = streamOf(intent);
                if (uri != null) {
                    keep(context, uri);
                } else {
                    CharSequence text = intent.getCharSequenceExtra(Intent.EXTRA_TEXT);
                    // Two shares before the application takes them: both,
                    // one after the other, not only the last.
                    if (text != null && text.length() > 0) {
                        sharedText = sharedText == null
                                ? text.toString()
                                : sharedText + "\n\n" + text;
                    }
                }
                break;
            }
            case Intent.ACTION_SEND_MULTIPLE: {
                ArrayList<Uri> uris = streamsOf(intent);
                if (uris != null) {
                    for (Uri uri : uris) {
                        if (uri != null) keep(context, uri);
                    }
                }
                break;
            }
            case Intent.ACTION_VIEW: {
                Uri uri = intent.getData();
                if (uri != null) keep(context, uri);
                break;
            }
            default:
                break;
        }
    }

    @SuppressWarnings("deprecation")
    private static Uri streamOf(Intent intent) {
        if (Build.VERSION.SDK_INT >= 33) {
            return intent.getParcelableExtra(Intent.EXTRA_STREAM, Uri.class);
        }
        return intent.getParcelableExtra(Intent.EXTRA_STREAM);
    }

    @SuppressWarnings("deprecation")
    private static ArrayList<Uri> streamsOf(Intent intent) {
        if (Build.VERSION.SDK_INT >= 33) {
            return intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM, Uri.class);
        }
        return intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM);
    }

    /** The documents shared since the last call, as URI strings. */
    public static synchronized String[] takeSharedUris() {
        String[] uris = sharedUris.toArray(new String[0]);
        sharedUris.clear();
        return uris;
    }

    /** The text shared since the last call; null when there is none. */
    public static synchronized String takeSharedText() {
        String text = sharedText;
        sharedText = null;
        return text;
    }

    // ---- A folder the person picked (ACTION_OPEN_DOCUMENT_TREE)

    /**
     * The folder as a document: the picked tree's own folder, or a folder
     * in it already given as a document of the tree. The tree's document id
     * of a document in it is the tree's, not the document's own.
     */
    private static String documentIdOf(Uri uri) {
        // By the URI's own segments (tree/<id>/document/<id>), each decoded
        // alone: the decoded path of a folder under one named "document"
        // holds "/document/" too.
        List<String> segments = uri.getPathSegments();
        if (segments.size() >= 4 && "document".equals(segments.get(2))) {
            return DocumentsContract.getDocumentId(uri);
        }
        return DocumentsContract.getTreeDocumentId(uri);
    }

    private static Uri folderOf(String tree) {
        Uri uri = Uri.parse(tree);
        return DocumentsContract.buildDocumentUriUsingTree(uri, documentIdOf(uri));
    }

    /** One row of a folder listing; true to stop there. */
    private interface ChildRow {
        boolean take(Cursor cursor);
    }

    /**
     * Go over the documents in the folder, with their id and display name.
     * Returns false when it cannot be listed.
     */
    private static boolean eachChild(Context context, String tree, ChildRow row) {
        try {
            Uri uri = Uri.parse(tree);
            Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(
                    uri, documentIdOf(uri));
            try (Cursor cursor = context.getContentResolver().query(
                    children,
                    new String[] {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                                  DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                                  DocumentsContract.Document.COLUMN_MIME_TYPE},
                    null, null, null)) {
                if (cursor == null) return false;
                while (cursor.moveToNext()) {
                    if (row.take(cursor)) break;
                }
            }
            return true;
        } catch (RuntimeException e) {
            return false;
        }
    }

    /** The display names of the documents in the folder; null when it cannot be listed. */
    private static boolean isFolder(String mime) {
        return DocumentsContract.Document.MIME_TYPE_DIR.equals(mime);
    }

    public static String[] childNames(Context context, String tree) {
        List<String> names = new ArrayList<>();
        // Files only, as the desktop lists them: a folder's name is not one
        // a saved file can take, and is never replaced by one.
        boolean listed = eachChild(context, tree, cursor -> {
            if (!isFolder(cursor.getString(2))) names.add(cursor.getString(1));
            return false;
        });
        return listed ? names.toArray(new String[0]) : null;
    }

    /**
     * The folder and everything under it, breadth first, a folder before
     * what is in it: its display name first, then four strings for each
     * document under it -- "d" or "f", its path under the folder (its names
     * joined by "/", a "/" in a name made "_"), its URI, its size. At most
     * maxNodes documents and maxDepth folders deep; a last "!" says the walk
     * stopped there. Null when it cannot be listed (access lost, gone).
     */
    public static String[] treeDocuments(Context context, String tree,
                                         int maxNodes, int maxDepth) {
        try {
            Uri root = Uri.parse(tree);
            List<String> found = new ArrayList<>();
            found.add(displayNameOf(context, folderOf(tree)));
            ArrayDeque<String[]> pending = new ArrayDeque<>();
            // {document id, path under the folder, depth}
            pending.add(new String[] {documentIdOf(root), "", "0"});
            int nodes = 0;
            boolean truncated = false;
            java.util.HashSet<String> paths = new java.util.HashSet<>();
            while (!pending.isEmpty() && !truncated) {
                String[] at = pending.poll();
                int depth = Integer.parseInt(at[2]);
                Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(
                        root, at[0]);
                try (Cursor cursor = context.getContentResolver().query(
                        children,
                        new String[] {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                                      DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                                      DocumentsContract.Document.COLUMN_MIME_TYPE,
                                      DocumentsContract.Document.COLUMN_SIZE},
                        null, null, DocumentsContract.Document.COLUMN_DISPLAY_NAME)) {
                    if (cursor == null) return null;
                    // A cloud provider still fetching the list gives part of
                    // it: an archive of a folder never leaves a file out, so
                    // the folder cannot be read yet.
                    android.os.Bundle extras = cursor.getExtras();
                    if (extras != null
                            && (extras.getBoolean(DocumentsContract.EXTRA_LOADING, false)
                                || extras.getString(DocumentsContract.EXTRA_ERROR) != null)) {
                        return null;
                    }
                    while (cursor.moveToNext()) {
                        if (nodes >= maxNodes) {
                            truncated = true;
                            break;
                        }
                        String id = cursor.getString(0);
                        String name = cursor.getString(1);
                        if (name == null || name.isEmpty()) name = "_";
                        name = name.replace('/', '_');
                        String path = at[1].isEmpty() ? name : at[1] + "/" + name;
                        // Made the same as another's by the "/" taken out
                        // ("a/b" and "a_b"): numbered, never merged into it.
                        for (int n = 2; !paths.add(path); ++n) {
                            path = (at[1].isEmpty() ? "" : at[1] + "/") + name + " (" + n + ")";
                        }
                        boolean folder = DocumentsContract.Document.MIME_TYPE_DIR
                                .equals(cursor.getString(2));
                        long size = cursor.isNull(3) ? 0 : cursor.getLong(3);
                        found.add(folder ? "d" : "f");
                        found.add(path);
                        found.add(DocumentsContract.buildDocumentUriUsingTree(
                                root, id).toString());
                        found.add(Long.toString(folder ? 0 : size));
                        ++nodes;
                        if (folder && depth + 1 < maxDepth) {
                            pending.add(new String[] {id, path,
                                                      Integer.toString(depth + 1)});
                        } else if (folder) {
                            truncated = true;
                        }
                    }
                }
            }
            if (truncated) found.add("!");
            return found.toArray(new String[0]);
        } catch (RuntimeException e) {
            return null;
        }
    }

    private static String displayNameOf(Context context, Uri document) {
        try (Cursor cursor = context.getContentResolver().query(
                document,
                new String[] {DocumentsContract.Document.COLUMN_DISPLAY_NAME},
                null, null, null)) {
            if (cursor != null && cursor.moveToFirst() && !cursor.isNull(0)) {
                return cursor.getString(0);
            }
        } catch (RuntimeException e) {
            // Named by its path below.
        }
        String id = documentIdOf(document);
        int cut = Math.max(id.lastIndexOf('/'), id.lastIndexOf(':'));
        return cut >= 0 ? id.substring(cut + 1) : id;
    }

    /** A new folder in the folder; the provider numbers a name that is taken. */
    public static String createFolder(Context context, String tree, String name) {
        return createChild(context, tree, DocumentsContract.Document.MIME_TYPE_DIR,
                           name);
    }

    /** Delete a document (a folder with all in it) made in a picked folder. */
    public static boolean deleteDocument(Context context, String document) {
        try {
            return DocumentsContract.deleteDocument(context.getContentResolver(),
                                                    Uri.parse(document));
        } catch (FileNotFoundException | RuntimeException e) {
            return false;
        }
    }

    /**
     * Rename a document made in a picked folder; its new URI, or null when
     * the provider could not (a name taken, or no renaming at all).
     */
    public static String renameDocument(Context context, String document,
                                        String name) {
        try {
            Uri renamed = DocumentsContract.renameDocument(
                    context.getContentResolver(), Uri.parse(document), name);
            return renamed == null ? null : renamed.toString();
        } catch (FileNotFoundException | RuntimeException e) {
            return null;
        }
    }

    /** The document named name in the folder; null when there is none. */
    public static String findChild(Context context, String tree, String name) {
        String[] found = new String[1];
        eachChild(context, tree, cursor -> {
            if (isFolder(cursor.getString(2)) || !name.equals(cursor.getString(1))) {
                return false;
            }
            found[0] = DocumentsContract.buildDocumentUriUsingTree(
                    Uri.parse(tree), cursor.getString(0)).toString();
            return true;
        });
        return found[0];
    }

    /** A new document in the folder; the provider numbers a name that is taken. */
    public static String createChild(Context context, String tree, String mime,
                                     String name) {
        try {
            Uri made = DocumentsContract.createDocument(
                    context.getContentResolver(), folderOf(tree), mime, name);
            return made == null ? null : made.toString();
        } catch (FileNotFoundException | RuntimeException e) {
            return null;
        }
    }

    /**
     * Start the application again in a new process. RestartActivity, in a
     * process of its own, ends this one and opens the launcher's activity.
     * Returns false when it could not be started.
     */
    private static boolean grantsReleased;

    // Once per process, before anything is picked: Qt's file and folder
    // dialogs take a persistent grant for every document they return, which
    // nothing needs past the process (a pick lasts the session), and Android
    // caps how many an app may hold before it refuses new ones.
    public static synchronized void releasePersistedGrants(Context context) {
        if (grantsReleased) return;
        grantsReleased = true;
        ContentResolver resolver = context.getContentResolver();
        for (UriPermission grant : resolver.getPersistedUriPermissions()) {
            int flags = (grant.isReadPermission()
                            ? Intent.FLAG_GRANT_READ_URI_PERMISSION : 0)
                    | (grant.isWritePermission()
                            ? Intent.FLAG_GRANT_WRITE_URI_PERMISSION : 0);
            try {
                resolver.releasePersistableUriPermission(grant.getUri(), flags);
            } catch (SecurityException ignored) {
                // Gone already: the provider or the document.
            }
        }
    }

    public static boolean restart(Context context) {
        Intent launch = context.getPackageManager()
                .getLaunchIntentForPackage(context.getPackageName());
        if (launch == null) return false;
        Intent intent = new Intent(context, RestartActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtra(RestartActivity.EXTRA_PID, android.os.Process.myPid());
        intent.putExtra(RestartActivity.EXTRA_LAUNCH, launch);
        try {
            context.startActivity(intent);
            return true;
        } catch (RuntimeException e) {
            return false;
        }
    }
}
