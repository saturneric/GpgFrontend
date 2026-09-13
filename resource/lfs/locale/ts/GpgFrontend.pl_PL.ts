<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="pl_PL">
<context>
    <name>GpgFrontend</name>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1372"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1374"/>
        <source>Expiring Soon</source>
        <translation>Wygasa wkrótce</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1376"/>
        <source>Expired</source>
        <translation>Wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1378"/>
        <source>Revoked</source>
        <translation>Unieważniony</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1380"/>
        <source>Disabled</source>
        <translation>Wyłączony</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgDecryptResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="40"/>
        <source>Decrypt</source>
        <translation>Odszyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="43"/>
        <source>Decrypt Operation</source>
        <translation>Operacja odszyfrowywania</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="46"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="48"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="52"/>
        <source>Detail</source>
        <translation>Szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="57"/>
        <source>Unsupported Algo</source>
        <translation>Nieobsługiwany algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="67"/>
        <source>General State</source>
        <translation>Stan ogólny</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="75"/>
        <source>File Name</source>
        <translation>Nazwa pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="78"/>
        <source>MIME</source>
        <translation>MIME</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="79"/>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="82"/>
        <source>false</source>
        <translation>nie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="79"/>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="82"/>
        <source>true</source>
        <translation>tak</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="81"/>
        <source>Message Integrity Protection</source>
        <translation>Ochrona integralności wiadomości</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="89"/>
        <source>Symmetric Encryption Algorithm</source>
        <translation>Algorytm szyfrowania symetrycznego</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="96"/>
        <source>Recipient(s)</source>
        <translation>Odbiorcy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="103"/>
        <source>Recipient</source>
        <translation>Odbiorca</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="118"/>
        <source>Decrypted successfully. The original filename is &quot;%1&quot;.</source>
        <translation>Odszyfrowanie zakończone pomyślnie. Oryginalna nazwa pliku to „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="121"/>
        <source>The message has been decrypted successfully and is now readable.</source>
        <translation>Wiadomość została pomyślnie odszyfrowana i jest teraz czytelna.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="127"/>
        <source>Decrypted, but message integrity protection is absent. This may be a legacy message that cannot detect tampering.</source>
        <translation>Odszyfrowano, ale wiadomość nie ma ochrony integralności. Może to być starsza wiadomość, w której nie da się wykryć manipulacji.</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="136"/>
        <source>Decrypted, but %n recipient key(s) could not be found in your keyring.</source>
        <translation>
            <numerusform>Odszyfrowano, ale %n klucz odbiorcy nie został odnaleziony w Twojej bazie kluczy.</numerusform>
            <numerusform>Odszyfrowano, ale %n klucze odbiorców nie zostały odnalezione w Twojej bazie kluczy.</numerusform>
            <numerusform>Odszyfrowano, ale %n kluczy odbiorców nie zostało odnalezionych w Twojej bazie kluczy.</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="139"/>
        <source>Decrypted with warnings. Please review the details.</source>
        <translation>Odszyfrowano z ostrzeżeniami. Przejrzyj szczegóły.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="144"/>
        <source>Decryption failed: %1.</source>
        <translation>Odszyfrowanie nie powiodło się: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="171"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;nieznany&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="179"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="183"/>
        <source>Subkey</source>
        <translation>Podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="184"/>
        <source>Primary Key</source>
        <translation>Klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="190"/>
        <source>Public Key Algo</source>
        <translation>Algorytm klucza publicznego</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="192"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgEncryptResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="40"/>
        <source>Encrypt</source>
        <translation>Szyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="43"/>
        <source>Encrypt Operation</source>
        <translation>Operacja szyfrowania</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="46"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="48"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="51"/>
        <source>Detail</source>
        <translation>Szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="63"/>
        <source>Invalid Recipients</source>
        <translation>Nieprawidłowi odbiorcy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="70"/>
        <source>Recipients</source>
        <translation>Odbiorcy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="72"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="74"/>
        <source>Reason</source>
        <translation>Powód</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="92"/>
        <source>Encrypted with warnings. Some recipients could not be added. Please review the invalid recipient list.</source>
        <translation>Zaszyfrowano z ostrzeżeniami. Nie udało się dodać niektórych odbiorców. Przejrzyj listę nieprawidłowych odbiorców.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="94"/>
        <source>Your data has been encrypted. Only the intended recipients can decrypt and read it.</source>
        <translation>Twoje dane zostały zaszyfrowane. Tylko przeznaczeni odbiorcy mogą je odszyfrować i odczytać.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="99"/>
        <source>Encryption failed: %1.</source>
        <translation>Szyfrowanie nie powiodło się: %1.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgKeyTableModel</name>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="43"/>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="43"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="44"/>
        <source>Email Address</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="44"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="253"/>
        <source>Usage</source>
        <translation>Zastosowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="44"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="254"/>
        <source>Trust</source>
        <translation>Zaufanie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="45"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="45"/>
        <source>Create Date</source>
        <translation>Data utworzenia</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="45"/>
        <source>Expire Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="46"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="46"/>
        <source>Subkey(s)</source>
        <translation>Podklucze</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="46"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="255"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="47"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="114"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="174"/>
        <source>Never</source>
        <translation>Nigdy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="251"/>
        <source>ID</source>
        <translation>Identyfikator</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="252"/>
        <source>Algo</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="256"/>
        <source>No Comment</source>
        <translation>Brak komentarza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="262"/>
        <source>SubKeys (up to 8):</source>
        <translation>Podklucze (maks. 8):</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="268"/>
        <source>ID: %1 | Algo: %2 | Usage: %3</source>
        <translation>Identyfikator: %1 | Algorytm: %2 | Zastosowanie: %3</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgKeyTreeModel</name>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="56"/>
        <source>Select</source>
        <translation>Wybierz</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="57"/>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="58"/>
        <source>Identity</source>
        <translation>Tożsamość</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="59"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="60"/>
        <source>Usage</source>
        <translation>Zastosowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="61"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="62"/>
        <source>Create Date</source>
        <translation>Data utworzenia</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="336"/>
        <source>(already shown above)</source>
        <translation>(pokazano już powyżej)</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgSignResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="44"/>
        <source>Sign</source>
        <translation>Podpisanie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="47"/>
        <source>Sign Operation</source>
        <translation>Operacja podpisywania</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="50"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="52"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="55"/>
        <source>Detail</source>
        <translation>Szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="66"/>
        <source>New Signature</source>
        <translation>Nowy podpis</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="69"/>
        <source>Sign Mode</source>
        <translation>Tryb podpisu</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="89"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="103"/>
        <source>Signed By</source>
        <translation>Podpisano przez</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="93"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="96"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="94"/>
        <source>Subkey</source>
        <translation>Podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="97"/>
        <source>Primary Key</source>
        <translation>Klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="99"/>
        <source>Key Create Date</source>
        <translation>Data utworzenia klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="103"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="139"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="104"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;nieznany&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="107"/>
        <source>Public Key Algo</source>
        <translation>Algorytm klucza publicznego</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="109"/>
        <source>Hash Algo</source>
        <translation>Algorytm skrótu</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="111"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="115"/>
        <source>Sign Date</source>
        <translation>Data podpisu</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="111"/>
        <source>UTC</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="115"/>
        <source>Localized</source>
        <translation>Czas lokalny</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="131"/>
        <source>Invalid Signers</source>
        <translation>Nieprawidłowi podpisujący</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="137"/>
        <source>Signer</source>
        <translation>Podpisujący</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="142"/>
        <source>Reason</source>
        <translation>Powód</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="156"/>
        <source>Signed by %1. Recipients can verify this data came from you and was not altered.</source>
        <translation>Podpisano przez %1. Odbiorcy mogą zweryfikować, że te dane pochodzą od Ciebie i nie zostały zmienione.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="158"/>
        <source>, </source>
        <translation>, </translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="161"/>
        <source>A digital signature has been created. Recipients can verify this data came from you and was not altered.</source>
        <translation>Utworzono podpis cyfrowy. Odbiorcy mogą zweryfikować, że te dane pochodzą od Ciebie i nie zostały zmienione.</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="167"/>
        <source>Signing completed, but %n signer(s) could not be used. Please review the details.</source>
        <translation>
            <numerusform>Podpisywanie zakończone, ale nie udało się użyć %n podpisującego. Przejrzyj szczegóły.</numerusform>
            <numerusform>Podpisywanie zakończone, ale nie udało się użyć %n podpisujących. Przejrzyj szczegóły.</numerusform>
            <numerusform>Podpisywanie zakończone, ale nie udało się użyć %n podpisujących. Przejrzyj szczegóły.</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="172"/>
        <source>Signing completed with warnings. Please review the details.</source>
        <translation>Podpisywanie zakończone z ostrzeżeniami. Przejrzyj szczegóły.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="177"/>
        <source>Signing failed: %1.</source>
        <translation>Podpisywanie nie powiodło się: %1.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgVerifyResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="45"/>
        <source>Verify Operation</source>
        <translation>Operacja weryfikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="48"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="50"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="63"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="68"/>
        <source>Signed On</source>
        <translation>Podpisano</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="63"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="371"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="411"/>
        <source>UTC</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="68"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="374"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="414"/>
        <source>Localized</source>
        <translation>Czas lokalny</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="74"/>
        <source>Signatures List</source>
        <translation>Lista podpisów</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="81"/>
        <source>Signature [%1]:</source>
        <translation>Podpis [%1]:</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="82"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="132"/>
        <source>Tips</source>
        <translation>Wskazówki</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="215"/>
        <source>Could not find information that can be used for verification.</source>
        <translation>Nie można znaleźć informacji, których można użyć do weryfikacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="363"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="389"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="402"/>
        <source>Signed By</source>
        <translation>Podpisano przez</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="363"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="402"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="365"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="403"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;nieznany&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="368"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="408"/>
        <source>Public Key Algo</source>
        <translation>Algorytm klucza publicznego</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="370"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="410"/>
        <source>Hash Algo</source>
        <translation>Algorytm skrótu</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="371"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="374"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="411"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="414"/>
        <source>Sign Date</source>
        <translation>Data podpisu</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="140"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="392"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="395"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="42"/>
        <source>Verify</source>
        <translation>Weryfikacja</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="53"/>
        <source>Detail</source>
        <translation>Szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="85"/>
        <source>Invalid Signature</source>
        <translation>Nieprawidłowy podpis</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="87"/>
        <source>This signature could not be verified</source>
        <translation>Nie można zweryfikować tego podpisu</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="99"/>
        <source>Fully Valid</source>
        <translation>W pełni prawidłowy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="101"/>
        <source>Valid but with Issues</source>
        <translation>Prawidłowy, ale z problemami</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="103"/>
        <source>Valid but Not Fully Trusted</source>
        <translation>Prawidłowy, ale bez pełnego zaufania</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="109"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="257"/>
        <source>Signature has expired</source>
        <translation>Podpis wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="112"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="259"/>
        <source>Signing key is missing</source>
        <translation>Brak klucza podpisującego</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="115"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="261"/>
        <source>Signing key has been revoked</source>
        <translation>Klucz podpisujący został unieważniony</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="118"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="263"/>
        <source>Signing key has expired</source>
        <translation>Klucz podpisujący wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="121"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="265"/>
        <source>Certificate revocation list is missing</source>
        <translation>Brak listy unieważnionych certyfikatów</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="125"/>
        <source>Warnings</source>
        <translation>Ostrzeżenia</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="133"/>
        <source>Adjust trust level to make it fully valid</source>
        <translation>Dostosuj poziom zaufania, aby podpis był w pełni prawidłowy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="141"/>
        <source>not present in keyring</source>
        <translation>brak w bazie kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="148"/>
        <source>Cannot Verify due to Key Missing</source>
        <translation>Nie można zweryfikować z powodu braku klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="150"/>
        <source>The public key needed to verify this signature is not available</source>
        <translation>Klucz publiczny potrzebny do zweryfikowania tego podpisu jest niedostępny</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="159"/>
        <source>Key Revoked</source>
        <translation>Klucz unieważniony</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="161"/>
        <source>The signature is mathematically valid but the signing key has been revoked</source>
        <translation>Podpis jest matematycznie prawidłowy, ale klucz podpisujący został unieważniony</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="170"/>
        <source>Signature Expired</source>
        <translation>Podpis wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="171"/>
        <source>The signature is valid but has expired</source>
        <translation>Podpis jest prawidłowy, ale wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="179"/>
        <source>Signing Key Expired</source>
        <translation>Klucz podpisujący wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="182"/>
        <source>The signature is valid but the signing key has expired</source>
        <translation>Podpis jest prawidłowy, ale klucz podpisujący wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="189"/>
        <source>Verification Error</source>
        <translation>Błąd weryfikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="191"/>
        <source>An error occurred during signature verification</source>
        <translation>Wystąpił błąd podczas weryfikacji podpisu</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="198"/>
        <source>Unknown Error</source>
        <translation>Nieznany błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="199"/>
        <source>Key Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="206"/>
        <source>Verification process stopped due to errors.</source>
        <translation>Proces weryfikacji został zatrzymany z powodu błędów.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="219"/>
        <source>No verifiable signatures were found in the data.</source>
        <translation>W danych nie znaleziono podpisów, które można zweryfikować.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="311"/>
        <source>The signature is fully valid and trusted. The data has not been tampered with.</source>
        <translation>Podpis jest w pełni prawidłowy i zaufany. Dane nie zostały zmodyfikowane.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="313"/>
        <source>The signature by %1 is fully valid and trusted. The data has not been tampered with.</source>
        <translation>Podpis złożony przez %1 jest w pełni prawidłowy i zaufany. Dane nie zostały zmodyfikowane.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="318"/>
        <source>The signature is valid, but the signing key is not fully trusted. You may need to set a higher trust level.</source>
        <translation>Podpis jest prawidłowy, ale klucz podpisujący nie jest w pełni zaufany. Może być konieczne ustawienie wyższego poziomu zaufania.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="324"/>
        <source>Verification incomplete. The signing key is not in your keyring. Please import the signer&apos;s public key.</source>
        <translation>Weryfikacja niekompletna. Klucz podpisującego nie znajduje się w Twojej bazie kluczy. Zaimportuj klucz publiczny podpisującego.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="328"/>
        <source>The signature is valid but the signing key is not fully trusted. Consider setting a higher trust level for that key.</source>
        <translation>Podpis jest prawidłowy, ale klucz podpisujący nie jest w pełni zaufany. Rozważ ustawienie wyższego poziomu zaufania dla tego klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="332"/>
        <source>Verification completed with warnings. Please review the details.</source>
        <translation>Weryfikacja zakończona z ostrzeżeniami. Przejrzyj szczegóły.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="338"/>
        <source>Invalid signature detected. The data may have been tampered with or the signature is corrupt.</source>
        <translation>Wykryto nieprawidłowy podpis. Dane mogły zostać zmodyfikowane lub podpis jest uszkodzony.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="342"/>
        <source>Signature verification failed. The signing key has been revoked.</source>
        <translation>Weryfikacja podpisu nie powiodła się. Klucz podpisujący został unieważniony.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="346"/>
        <source>Signature verification failed. This signature has expired.</source>
        <translation>Weryfikacja podpisu nie powiodła się. Ten podpis wygasł.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="349"/>
        <source>Signature verification failed. The signing key has expired.</source>
        <translation>Weryfikacja podpisu nie powiodła się. Klucz podpisujący wygasł.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="352"/>
        <source>Cannot verify. The signing key is not available in your keyring.</source>
        <translation>Nie można zweryfikować. Klucz podpisujący nie jest dostępny w Twojej bazie kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="356"/>
        <source>Signature verification failed. Please review the details.</source>
        <translation>Weryfikacja podpisu nie powiodła się. Przejrzyj szczegóły.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="393"/>
        <source>Subkey</source>
        <translation>Podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="396"/>
        <source>Primary Key</source>
        <translation>Klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="398"/>
        <source>Key Create Date</source>
        <translation>Data utworzenia klucza</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::KeyGenerateInfo</name>
    <message>
        <location filename="../../../../src/core/model/GpgKeyGenerateInfo.cpp" line="186"/>
        <source>None</source>
        <translation>Brak</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::Module::GlobalRegisterTableTreeModel::Impl</name>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="236"/>
        <source>Leaf</source>
        <translation>Liść</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="236"/>
        <source>Namespace</source>
        <translation>Przestrzeń nazw</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="264"/>
        <source>Empty</source>
        <translation>Puste</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="268"/>
        <source>String</source>
        <translation>Tekst</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="270"/>
        <source>Boolean</source>
        <translation>Wartość logiczna</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="275"/>
        <source>Integer</source>
        <translation>Liczba całkowita</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="278"/>
        <source>Number</source>
        <translation>Liczba</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="280"/>
        <source>Unsupported</source>
        <translation>Nieobsługiwany</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="323"/>
        <source>&lt;UNSUPPORTED&gt;</source>
        <translation>&lt;NIEOBSŁUGIWANY&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="358"/>
        <source>Key</source>
        <translation>Klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="360"/>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="362"/>
        <source>Value Type</source>
        <translation>Typ wartości</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="364"/>
        <source>Value</source>
        <translation>Wartość</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="366"/>
        <source>Version</source>
        <translation>Wersja</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI</name>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="234"/>
        <source>Short Name</source>
        <translation>Krótka nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="235"/>
        <source>The name &quot;%1&quot; is shorter than five characters. Short names are allowed, but they are often a typo and make the key harder for others to recognise.

Do you want to continue?</source>
        <translation>Nazwa „%1” jest krótsza niż pięć znaków. Krótkie nazwy są dozwolone, ale często są literówką i utrudniają innym rozpoznanie klucza.

Czy chcesz kontynuować?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="50"/>
        <source>Signature (OPENPGP.1)</source>
        <translation>Podpis (OPENPGP.1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="53"/>
        <source>Encryption (OPENPGP.2)</source>
        <translation>Szyfrowanie (OPENPGP.2)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="56"/>
        <source>Authentication (OPENPGP.3)</source>
        <translation>Uwierzytelnianie (OPENPGP.3)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="75"/>
        <source>Select Card Slot</source>
        <translation>Wybierz slot karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="76"/>
        <source>This key can be stored in more than one slot. Where should it be stored?</source>
        <translation>Ten klucz może być przechowywany w więcej niż jednym slocie. Gdzie powinien być przechowywany?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="97"/>
        <source>No Smart Card</source>
        <translation>Brak karty inteligentnej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="98"/>
        <source>No OpenPGP smart card was detected. Insert a card and try again.</source>
        <translation>Nie wykryto karty inteligentnej OpenPGP. Włóż kartę i spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="108"/>
        <source>Select Smart Card</source>
        <translation>Wybierz kartę inteligentną</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="109"/>
        <source>Move the key to which card?</source>
        <translation>Na którą kartę przenieść klucz?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="124"/>
        <source>Back Up Secret Key</source>
        <translation>Kopia zapasowa klucza prywatnego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="125"/>
        <source>Do you want to export a backup of the secret key before moving it to the card? After the move the key can only be used through the card.</source>
        <translation>Czy chcesz wyeksportować kopię zapasową klucza prywatnego przed przeniesieniem go na kartę? Po przeniesieniu klucz będzie można używać tylko przez kartę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="130"/>
        <source>Back Up First</source>
        <translation>Najpierw kopia zapasowa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="133"/>
        <source>Continue Without Backup</source>
        <translation>Kontynuuj bez kopii zapasowej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="155"/>
        <source>Export Secret Key Backup</source>
        <translation>Eksportuj kopię zapasową klucza prywatnego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="158"/>
        <source>Key Files</source>
        <translation>Pliki kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="164"/>
        <source>Export Error</source>
        <translation>Błąd eksportu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="165"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Nie można otworzyć %1 do zapisu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="187"/>
        <source>Move Key to Smart Card</source>
        <translation>Przenieś klucz na kartę inteligentną</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="191"/>
        <source>&lt;h3&gt;You are about to move a private key onto a smart card.&lt;/h3&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br/&gt;&lt;br/&gt;This &lt;b&gt;moves&lt;/b&gt; the key: its private part is removed from this computer and only a card reference (stub) remains. Afterwards the key can only be used through the card. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br/&gt;&lt;br/&gt;Do you want to continue?</source>
        <translation>&lt;h3&gt;Zamierzasz przenieść klucz prywatny na kartę inteligentną.&lt;/h3&gt;&lt;b&gt;Identyfikator klucza:&lt;/b&gt; %1&lt;br/&gt;&lt;br/&gt;To działanie &lt;b&gt;przenosi&lt;/b&gt; klucz: jego część prywatna zostaje usunięta z tego komputera i pozostaje tylko odniesienie do karty (zaślepka). Od tej pory klucz będzie można używać tylko przez kartę. To działanie jest &lt;b&gt;nieodwracalne&lt;/b&gt;.&lt;br/&gt;&lt;br/&gt;Czy chcesz kontynuować?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="214"/>
        <source>This key has no capability that can be stored on a smart card.</source>
        <translation>Ten klucz nie ma żadnych zdolności, które można przechować na karcie inteligentnej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="242"/>
        <source>The key was moved to the smart card successfully.</source>
        <translation>Klucz został pomyślnie przeniesiony na kartę inteligentną.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="93"/>
        <source>No Supported OpenPGP Engine Found</source>
        <translation>Nie znaleziono obsługiwanego silnika OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="95"/>
        <source>It seems that no supported OpenPGP engine is available. Please check your if GpgFrontend is properly installed and try again. Reason: %1</source>
        <translation>Wygląda na to, że żaden obsługiwany silnik OpenPGP nie jest dostępny. Sprawdź, czy GpgFrontend jest poprawnie zainstalowany, i spróbuj ponownie. Powód: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="40"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="53"/>
        <source>Failure</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="67"/>
        <source>Open Key</source>
        <translation>Otwórz klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="69"/>
        <source>Keyring files</source>
        <translation>Pliki baz kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="77"/>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="87"/>
        <location filename="../../../../src/ui/function/ShowKeyDetails.cpp" line="41"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="78"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Nie można otworzyć tego pliku. Upewnij się, że jest to zwykły plik i jest możliwy do odczytania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="88"/>
        <source>The target file is too large for a keyring.</source>
        <translation>Plik docelowy jest zbyt duży dla bazy kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="97"/>
        <source>File Open Failed</source>
        <translation>Otwarcie pliku nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="98"/>
        <source>Failed to open file: </source>
        <translation>Nie udało się otworzyć pliku: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ShowKeyDetails.cpp" line="42"/>
        <source>Key Not Found.</source>
        <translation>Nie znaleziono klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="54"/>
        <source>Gpg Operation failed.</source>
        <translation>Operacja GPG nie powiodła się.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="51"/>
        <source>No Usable Key Database</source>
        <translation>Brak użytecznej bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="53"/>
        <source>None of the configured key databases could be opened. This usually means the folder was moved or deleted, or is on a drive that is not currently available.</source>
        <translation>Nie można otworzyć żadnej ze skonfigurowanych baz kluczy. Zwykle oznacza to, że folder został przeniesiony lub usunięty albo znajduje się na dysku, który nie jest obecnie dostępny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="59"/>
        <source>You can change where your key databases live in Settings, under Key Databases. Details: %1</source>
        <translation>Lokalizację baz kluczy możesz zmienić w Ustawieniach, w sekcji Bazy kluczy. Szczegóły: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="68"/>
        <source>Cannot Prepare Application Data</source>
        <translation>Nie można przygotować danych aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="70"/>
        <source>GpgFrontend could not set up the folders it needs to store its data. Please check that the application data folder is writable. Details: %1</source>
        <translation>GpgFrontend nie mógł utworzyć folderów potrzebnych do przechowywania swoich danych. Sprawdź, czy folder danych aplikacji jest możliwy do zapisu. Szczegóły: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="79"/>
        <source>Key Database Could Not Be Opened</source>
        <translation>Nie można otworzyć bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="81"/>
        <source>The key database was found but could not be loaded. It may be in use by another program, or its permissions may have changed. Details: %1</source>
        <translation>Baza kluczy została znaleziona, ale nie udało się jej wczytać. Może być używana przez inny program lub jej uprawnienia mogły się zmienić. Szczegóły: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="41"/>
        <source>Operation completed successfully.</source>
        <translation>Operacja zakończona pomyślnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="56"/>
        <source>Error code: %1</source>
        <translation>Kod błędu: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="59"/>
        <source>Source:  %1</source>
        <translation>Źródło:  %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="62"/>
        <source>Description: %1</source>
        <translation>Opis: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="65"/>
        <source>Error Message: %1</source>
        <translation>Komunikat błędu: %1</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ADSKsPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="58"/>
        <source>Confirm</source>
        <translation>Potwierdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="59"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="70"/>
        <source>No Subkeys Selected</source>
        <translation>Nie wybrano podkluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="71"/>
        <source>Please select at least one Subkey.</source>
        <translation>Wybierz co najmniej jeden podklucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="81"/>
        <source>Select ADSK(s)</source>
        <translation>Wybierz ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="86"/>
        <source>ADSK (Additional Decryption Subkey) allows others to encrypt data for you without having access to your private key. You are only allow to check subkeys with encryption capability.</source>
        <translation>ADSK (dodatkowy podklucz odszyfrowujący) pozwala innym szyfrować dane dla Ciebie bez dostępu do Twojego klucza prywatnego. Zaznaczać wolno tylko podklucze z możliwością szyfrowania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="110"/>
        <source>ADSKs Picker</source>
        <translation>Wybór ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="130"/>
        <source>Key ID: %1 Reason: %2</source>
        <translation>Identyfikator klucza: %1 Powód: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="142"/>
        <source>Unknown Error</source>
        <translation>Nieznany błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="143"/>
        <source>Caught unknown error while exporting the key.</source>
        <translation>Wystąpił nieznany błąd podczas eksportowania klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="160"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="161"/>
        <source>Partially Failed</source>
        <translation>Częściowe niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="164"/>
        <source>Failed to add all selected subkeys.</source>
        <translation>Nie udało się dodać wszystkich wybranych podkluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="165"/>
        <source>Some subkeys failed to be added as ADSKs.</source>
        <translation>Niektóre podklucze nie zostały dodane jako ADSK.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AboutDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="192"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="202"/>
        <source>About</source>
        <translation>O programie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="203"/>
        <source>Build Information</source>
        <translation>Informacje o kompilacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="205"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="693"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="208"/>
        <source>Rust Engine</source>
        <translation>Silnik Rust</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="204"/>
        <source>Translators</source>
        <translation>Tłumacze</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AboutStatusInfo</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="41"/>
        <source>Memory</source>
        <translation>Pamięć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="43"/>
        <source>Not written to this disk in the normal course of things.</source>
        <translation>W normalnych okolicznościach nie jest zapisywany na tym dysku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="50"/>
        <source>An encrypted folder</source>
        <translation>Zaszyfrowany folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="52"/>
        <source>Only this session can read it.</source>
        <translation>Tylko ta sesja może go odczytać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="57"/>
        <source>An ordinary folder on this disk</source>
        <translation>Zwykły folder na tym dysku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="59"/>
        <source>Nothing here could hold it in memory or encrypt it.</source>
        <translation>Nic tutaj nie przechowuje go w pamięci ani nie szyfruje.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="69"/>
        <source>Profile:</source>
        <translation>Profil:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="71"/>
        <source>Profile Type:</source>
        <translation>Typ profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="81"/>
        <source>The profile this computer starts on, in your user data folder.</source>
        <translation>Profil startowy tego komputera, przechowywany w Twoim folderze danych użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="92"/>
        <source>Kept beside the application, so it travels with it.</source>
        <translation>Przechowywany obok aplikacji, więc podróżuje razem z nią.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="105"/>
        <source>Opened from a profile file</source>
        <translation>Otwarty z pliku profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="109"/>
        <source>Temporary. It disappears when this window closes, and closing asks whether to save your changes back into the file.</source>
        <translation>Tymczasowy. Znika po zamknięciu tego okna, a przy zamykaniu pojawi się pytanie, czy zapisać zmiany z powrotem do pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="120"/>
        <source>Kept on this computer</source>
        <translation>Przechowywany na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="130"/>
        <source>System keychain</source>
        <translation>Systemowy brelok kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="134"/>
        <source>PIN at startup</source>
        <translation>PIN przy uruchomieniu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="138"/>
        <source>No extra protection</source>
        <translation>Brak dodatkowej ochrony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="148"/>
        <source>This profile can leave this computer, so the system keychain is not offered: a key sealed with one computer&apos;s keychain cannot be opened on another.</source>
        <translation>Ten profil może opuścić ten komputer, dlatego systemowy brelok kluczy nie jest oferowany: klucz zapieczętowany brelokiem kluczy jednego komputera nie może zostać otwarty na innym.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AdvancedTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="98"/>
        <source>Security</source>
        <translation>Bezpieczeństwo</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="113"/>
        <source>wipe freed memory</source>
        <translation>wymazywanie zwolnionej pamięci</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="115"/>
        <source>also lock memory pages</source>
        <translation>także blokowanie stron pamięci</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="117"/>
        <source>also rotate keys weekly</source>
        <translation>także cotygodniowa rotacja kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="119"/>
        <source>no extra hardening</source>
        <translation>brak dodatkowego wzmocnienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="129"/>
        <source>How aggressively the application protects your secrets. Higher levels cost some performance.</source>
        <translation>Określa, jak agresywnie aplikacja chroni Twoje sekrety. Wyższe poziomy kosztują trochę wydajności.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="131"/>
        <source>Secure Level:</source>
        <translation>Poziom bezpieczeństwa:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="137"/>
        <source>No extra protection (default)</source>
        <translation>Brak dodatkowej ochrony (domyślne)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="140"/>
        <source>System keychain</source>
        <translation>Systemowy brelok kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="142"/>
        <source>PIN at startup</source>
        <translation>PIN przy uruchomieniu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="145"/>
        <source>How the application key file is protected on disk. The system keychain keeps a secret on this computer and never asks you for a password; a PIN is asked for each time the application starts.</source>
        <translation>Określa, jak plik klucza aplikacji jest chroniony na dysku. Systemowy brelok kluczy przechowuje sekret na tym komputerze i nigdy nie prosi Cię o hasło; PIN jest pytany przy każdym uruchomieniu aplikacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="149"/>
        <source>Change PIN…</source>
        <translation>Zmień PIN…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="154"/>
        <source>Application Key Protection:</source>
        <translation>Ochrona klucza aplikacji:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="157"/>
        <source>Weekly key rotation offers little protection while the application key itself is stored unprotected on disk. Consider using the system keychain or a PIN.</source>
        <translation>Cotygodniowa rotacja kluczy daje niewielką ochronę, dopóki sam klucz aplikacji jest przechowywany na dysku bez zabezpieczeń. Rozważ użycie systemowego breloka kluczy lub PIN-u.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="165"/>
        <source>Diagnostics</source>
        <translation>Diagnostyka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="169"/>
        <source>Debug</source>
        <translation>Debugowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="170"/>
        <source>Info</source>
        <translation>Informacje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="171"/>
        <source>Warning</source>
        <translation>Ostrzeżenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="173"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="175"/>
        <source>Fatal</source>
        <translation>Krytyczny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="177"/>
        <source>The least severe message that still gets written to the log. Debug is the most detailed and writes the most to disk.</source>
        <translation>Najmniej poważny komunikat, który jest jeszcze zapisywany do dziennika. Debugowanie jest najbardziej szczegółowe i zapisuje na dysku najwięcej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="179"/>
        <source>Log Level:</source>
        <translation>Poziom dziennika:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="183"/>
        <source> entries</source>
        <translation> wpisów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="185"/>
        <source>How many recent log messages are kept in memory for crash reports and the log viewer. Larger values use more memory.</source>
        <translation>Ile ostatnich komunikatów dziennika jest przechowywanych w pamięci na potrzeby raportów awarii i przeglądarki dziennika. Większe wartości zużywają więcej pamięci.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="187"/>
        <source>Log Ring Buffer:</source>
        <translation>Bufor pierścieniowy dziennika:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="190"/>
        <source>These settings are read once while the application starts, so a change only takes effect after a restart.</source>
        <translation>Te ustawienia są odczytywane raz podczas uruchamiania aplikacji, więc zmiana zadziała dopiero po ponownym uruchomieniu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="196"/>
        <source>Some settings on this page are fixed by the ENV.ini file next to the application and cannot be changed here. Edit that file to change them.</source>
        <translation>Niektóre ustawienia na tej stronie są ustalone przez plik ENV.ini obok aplikacji i nie można ich tutaj zmienić. Aby je zmienić, edytuj ten plik.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="245"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="498"/>
        <source>System Keychain Unavailable</source>
        <translation>Systemowy brelok kluczy niedostępny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="246"/>
        <source>The system credential store could not be used, so the application key cannot be protected with it.</source>
        <translation>Nie udało się użyć systemowego magazynu poświadczeń, więc klucz aplikacji nie może być nim chroniony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="249"/>
        <source>On Linux this needs a running secret service, such as GNOME Keyring or KWallet, and it must be unlocked.</source>
        <translation>W systemie Linux wymaga to działającej usługi sekretów, takiej jak GNOME Keyring lub KWallet, która musi być odblokowana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="279"/>
        <source>Turn Off Weekly Key Rotation?</source>
        <translation>Wyłączyć cotygodniową rotację kluczy?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="280"/>
        <source>At the %1 level the application saves your data with a key that changes every week. Choosing a lower level stops loading those keys, so anything saved while this level was on can no longer be read and is deleted after a short grace period.</source>
        <translation>Na poziomie %1 aplikacja zapisuje Twoje dane kluczem, który zmienia się co tydzień. Wybór niższego poziomu przestaje wczytywać te klucze, więc wszystko zapisane, gdy ten poziom był włączony, przestanie być czytelne i zostanie usunięte po krótkim okresie karencji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="286"/>
        <source>Lower the level anyway?</source>
        <translation>Czy mimo to obniżyć poziom?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="313"/>
        <source>Fixed by ENV.ini and cannot be changed here.</source>
        <translation>Ustalone przez ENV.ini — nie można tego tutaj zmienić.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="385"/>
        <source>This profile was opened from a file, and the passphrase that opens that file also protects its key. To change it, export the profile again with a new passphrase.</source>
        <translation>Ten profil został otwarty z pliku, a hasło otwierające ten plik chroni również jego klucz. Aby je zmienić, wyeksportuj profil ponownie z nowym hasłem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="400"/>
        <source>Not available for a profile file: the file is meant to be opened on another computer, possibly running another operating system, where a secret stored in this computer&apos;s credential store cannot be read. Use a PIN, or no protection.</source>
        <translation>Niedostępne dla pliku profilu: plik jest przeznaczony do otwierania na innym komputerze, prawdopodobnie z innym systemem operacyjnym, gdzie nie można odczytać sekretu przechowywanego w magazynie poświadczeń tego komputera. Użyj PIN-u albo braku ochrony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="405"/>
        <source>Not available in portable mode: a portable installation must not depend on secrets stored on one particular computer.</source>
        <translation>Niedostępne w trybie przenośnym: instalacja przenośna nie może zależeć od sekretów przechowywanych na jednym konkretnym komputerze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="413"/>
        <source>No system credential store is available on this computer. On Linux this needs a running secret service, such as GNOME Keyring or KWallet, and the libsecret client library (libsecret-1-0 on Debian and Ubuntu, libsecret on Fedora).</source>
        <translation>Na tym komputerze nie jest dostępny żaden systemowy magazyn poświadczeń. W systemie Linux wymaga to działającej usługi sekretów, takiej jak GNOME Keyring lub KWallet, oraz biblioteki klienckiej libsecret (libsecret-1-0 w Debianie i Ubuntu, libsecret w Fedorze).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="499"/>
        <source>The application key could not be protected with the system keychain, so it has been left exactly as it was.</source>
        <translation>Nie udało się chronić klucza aplikacji systemowym brelokiem kluczy, więc pozostawiono go dokładnie tak, jak był.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="503"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="553"/>
        <source>Application Key Error</source>
        <translation>Błąd klucza aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="504"/>
        <source>The application key could not be re-protected: %1.</source>
        <translation>Nie udało się ponownie chronić klucza aplikacji: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="507"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="555"/>
        <source>It has been left exactly as it was, so nothing has been lost.</source>
        <translation>Został pozostawiony dokładnie tak, jak był, więc nic nie zostało utracone.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="536"/>
        <source>The current PIN is not correct.</source>
        <translation>Bieżący PIN jest nieprawidłowy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="547"/>
        <source>PIN Changed</source>
        <translation>PIN zmieniony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="548"/>
        <source>The application PIN has been changed.</source>
        <translation>PIN aplikacji został zmieniony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="554"/>
        <source>The PIN could not be changed: %1.</source>
        <translation>Nie udało się zmienić PIN-u: %1.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AppKeyPinDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="62"/>
        <source>Unlock Application Key</source>
        <translation>Odblokuj klucz aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="65"/>
        <source>Change Application PIN</source>
        <translation>Zmień PIN aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="68"/>
        <source>Set an Application PIN</source>
        <translation>Ustaw PIN aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="74"/>
        <source>This PIN encrypts the application key on disk. You will be asked for it every time the application starts.</source>
        <translation>Ten PIN szyfruje klucz aplikacji na dysku. Aplikacja będzie o niego pytać przy każdym uruchomieniu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="77"/>
        <source>This application&apos;s key is protected by a PIN. Enter it to continue.</source>
        <translation>Klucz tej aplikacji jest chroniony PIN-em. Wprowadź go, aby kontynuować.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="82"/>
        <source>Current PIN</source>
        <translation>Bieżący PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="83"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="86"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="85"/>
        <source>New PIN</source>
        <translation>Nowy PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="87"/>
        <source>Confirm</source>
        <translation>Potwierdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="88"/>
        <source>Show PIN</source>
        <translation>Pokaż PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="89"/>
        <source>Strength</source>
        <translation>Siła</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="112"/>
        <source>If you forget this PIN, everything the application has encrypted becomes permanently unreadable. There is no recovery.</source>
        <translation>Jeśli zapomnisz tego PIN-u, wszystko, co aplikacja zaszyfrowała, stanie się trwale nieczytelne. Nie ma możliwości odzyskania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="106"/>
        <source>This PIN cannot be recovered if it is lost.</source>
        <translation>Tego PIN-u nie można odzyskać w razie jego utraty.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="104"/>
        <source>Use at least %1 characters.</source>
        <translation>Użyj co najmniej %1 znaków.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="92"/>
        <source>Unlock</source>
        <translation>Odblokuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="93"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="99"/>
        <source>Quit</source>
        <translation>Zakończ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="123"/>
        <source>Forgot PIN? Reset…</source>
        <translation>Nie pamiętasz PIN-u? Resetuj…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="118"/>
        <source>The PIN must be at least %1 characters.</source>
        <translation>PIN musi mieć co najmniej %1 znaków.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="119"/>
        <source>The two PINs do not match.</source>
        <translation>Oba PIN-y nie są zgodne.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AppearanceTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="93"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="94"/>
        <source>Theme</source>
        <translation>Motyw</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="96"/>
        <source>Toolbar</source>
        <translation>Pasek narzędzi</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="98"/>
        <source>Icon Size</source>
        <translation>Rozmiar ikon</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="99"/>
        <source>small</source>
        <translation>mały</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="100"/>
        <source>medium</source>
        <translation>średni</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="101"/>
        <source>large</source>
        <translation>duży</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="103"/>
        <source>Icon Style</source>
        <translation>Styl ikon</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="104"/>
        <source>just text</source>
        <translation>tylko tekst</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="105"/>
        <source>just icons</source>
        <translation>tylko ikony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="106"/>
        <source>text and icons</source>
        <translation>tekst i ikony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="108"/>
        <source>Actions</source>
        <translation>Akcje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="110"/>
        <source>IM actions turn the text into one compact line that is safe to paste into an instant messenger.</source>
        <translation>Akcje komunikatora zamieniają tekst w jedną zwartą linię, którą można bezpiecznie wkleić do komunikatora.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="114"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="126"/>
        <source>Font Family</source>
        <translation>Krój czcionki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="116"/>
        <source>Tab Size</source>
        <translation>Rozmiar tabulatora</translation>
    </message>
    <message>
        <source>Text Direction</source>
        <translation type="vanished">Kierunek tekstu</translation>
    </message>
    <message>
        <source>Automatic</source>
        <translation type="vanished">Automatycznie</translation>
    </message>
    <message>
        <source>Left-to-Right</source>
        <translation type="vanished">Od lewej do prawej</translation>
    </message>
    <message>
        <source>Right-to-Left</source>
        <translation type="vanished">Od prawej do lewej</translation>
    </message>
    <message>
        <source>Which way the message text runs. Automatic follows the first letter of the text, so a message written in Arabic, Hebrew or Persian reads from the right on its own. Applies to the editor tabs and to the status panel.</source>
        <translation type="vanished">Określa kierunek tekstu wiadomości. Tryb automatyczny podąża za pierwszą literą tekstu, więc wiadomość napisana po arabsku, hebrajsku lub persku sama czyta się od prawej. Dotyczy kart edytora i panelu statusu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="118"/>
        <source>Show all fonts</source>
        <translation>Pokaż wszystkie czcionki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="120"/>
        <source>Also offer proportional fonts for both surfaces below. They line up best with a monospaced one, but scripts such as Arabic are only shaped correctly by a font that covers them, and the monospaced font shipped with GpgFrontend covers Latin, Greek and Cyrillic only.</source>
        <translation>Pokazuje dodatkowo czcionki proporcjonalne dla obu powierzchni poniżej. Najlepiej współgrają z czcionką o stałej szerokości, ale pisma takie jak arabski są poprawnie kształtowane tylko przez czcionkę, która je obejmuje, a czcionka o stałej szerokości dołączona do GpgFrontend obejmuje jedynie łacinę, grekę i cyrylicę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="125"/>
        <source>Status Panel</source>
        <translation>Panel statusu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="115"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="127"/>
        <source>Font Size</source>
        <translation>Rozmiar czcionki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="113"/>
        <source>Text Editor</source>
        <translation>Edytor tekstu</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::BuildInfoTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="361"/>
        <source>GpgFrontend:</source>
        <translation>GpgFrontend:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="362"/>
        <source>Qt:</source>
        <translation>Qt:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="363"/>
        <source>GPGME:</source>
        <translation>GPGME:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="364"/>
        <source>Assuan:</source>
        <translation>Assuan:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="365"/>
        <source>Libarchive:</source>
        <translation>Libarchive:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="366"/>
        <source>OpenSSL:</source>
        <translation>OpenSSL:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="367"/>
        <source>Sodium:</source>
        <translation>Sodium:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="368"/>
        <source>Git Branch:</source>
        <translation>Gałąź Git:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="369"/>
        <source>Git Commit:</source>
        <translation>Commit Git:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="370"/>
        <source>Built at:</source>
        <translation>Zbudowano:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="374"/>
        <source>Copy Build Information</source>
        <translation>Skopiuj informacje o kompilacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="381"/>
        <source>Build Information</source>
        <translation>Informacje o kompilacji</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ChoosePage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="411"/>
        <source>Choose a guide</source>
        <translation>Wybierz przewodnik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="412"/>
        <source>Pick a topic if you want to learn the basics first.</source>
        <translation>Wybierz temat, jeśli najpierw chcesz poznać podstawy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="414"/>
        <source>Generate a new Key Pair</source>
        <translation>Wygeneruj nową parę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="415"/>
        <source>Learn how to create your own key pairs.</source>
        <translation>Dowiedz się, jak tworzyć własne pary kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="418"/>
        <source>Text operations</source>
        <translation>Operacje na tekście</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="419"/>
        <source>Learn how to encrypt, decrypt, sign, and verify text messages.</source>
        <translation>Dowiedz się, jak szyfrować, odszyfrowywać, podpisywać i weryfikować wiadomości tekstowe.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="422"/>
        <source>File operations</source>
        <translation>Operacje na plikach</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="423"/>
        <source>Learn how to encrypt, decrypt, sign, and verify files securely.</source>
        <translation>Dowiedz się, jak bezpiecznie szyfrować, odszyfrowywać, podpisywać i weryfikować pliki.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="426"/>
        <source>View key pair information</source>
        <translation>Podejrzyj informacje o parze kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="427"/>
        <source>Learn how to inspect key details, user IDs, fingerprints, and key capabilities.</source>
        <translation>Dowiedz się, jak przeglądać szczegóły kluczy, identyfikatory użytkownika, odciski kluczy i możliwości kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="431"/>
        <source>You can also skip these guides and start using GpgFrontend directly.</source>
        <translation>Możesz też pominąć te przewodniki i od razu zacząć korzystać z GpgFrontend.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ConclusionPage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="497"/>
        <source>Ready to use</source>
        <translation>Gotowe do użycia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="499"/>
        <source>GpgFrontend is ready. You can adjust these options before finishing.</source>
        <translation>GpgFrontend jest gotowy. Możesz dostosować te opcje przed zakończeniem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="503"/>
        <source>Contact and feedback</source>
        <translation>Kontakt i uwagi</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="504"/>
        <source>Report issues, ask questions, or send feedback to help improve GpgFrontend.</source>
        <translation>Zgłaszaj problemy, zadawaj pytania lub wysyłaj uwagi, aby pomóc ulepszać GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="508"/>
        <source>Submit an issue on GitHub</source>
        <translation>Zgłoś problem na GitHubie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="509"/>
        <source>Use GitHub issues if you want to report a bug or track a technical problem.</source>
        <translation>Skorzystaj z systemu zgłoszeń GitHub, jeśli chcesz zgłosić błąd lub śledzić problem techniczny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="514"/>
        <source>Don&apos;t show this setup wizard again</source>
        <translation>Nie pokazuj ponownie tego kreatora konfiguracji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="512"/>
        <source>Check for updates on startup</source>
        <translation>Sprawdzaj aktualizacje przy uruchomieniu</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::DocViewerDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="93"/>
        <source>Document Preview</source>
        <translation>Podgląd dokumentu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="94"/>
        <source>Document Preview — %1</source>
        <translation>Podgląd dokumentu — %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="126"/>
        <source>Zoom out</source>
        <translation>Pomniejsz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="130"/>
        <source>Zoom in</source>
        <translation>Powiększ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="134"/>
        <source>Fit to window</source>
        <translation>Dopasuj do okna</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="137"/>
        <source>Actual size (100%)</source>
        <translation>Rzeczywisty rozmiar (100%)</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::EncryptionKeysPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="80"/>
        <source>Select Encryption Key(s)</source>
        <translation>Wybierz klucze szyfrujące</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="85"/>
        <source>Choose Encryption Key(s) or Subkey(s)</source>
        <translation>Wybierz klucz lub podklucz szyfrujący</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="91"/>
        <source>Select the recipient key(s) or specific encryption subkey(s) to use for this operation. Expand a key to choose a particular subkey.</source>
        <translation>Wybierz klucze odbiorców lub konkretne podklucze szyfrujące do użycia w tej operacji. Rozwiń klucz, aby wybrać konkretny podklucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="105"/>
        <source>Confirm</source>
        <translation>Potwierdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="108"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="113"/>
        <source>No Key Selected</source>
        <translation>Nie wybrano klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="114"/>
        <source>Please select at least one encryption key or subkey.</source>
        <translation>Wybierz co najmniej jeden klucz lub podklucz szyfrujący.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ExportKey</name>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="116"/>
        <source>Unknown Error</source>
        <translation>Nieznany błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="117"/>
        <source>Caught unknown error while exporting the key.</source>
        <translation>Wystąpił nieznany błąd podczas eksportowania klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="135"/>
        <source>Export Key To File</source>
        <translation>Eksportuj klucz do pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="136"/>
        <source>Key Files</source>
        <translation>Pliki kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="142"/>
        <source>Export Error</source>
        <translation>Błąd eksportu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="143"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Nie można otworzyć %1 do zapisu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="148"/>
        <source>Export Successful</source>
        <translation>Eksport zakończony pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="149"/>
        <source>The key has been successfully exported to %1.</source>
        <translation>Klucz został pomyślnie wyeksportowany do %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="163"/>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="185"/>
        <source>WARNING: You are about to export your</source>
        <translation>OSTRZEŻENIE: Zamierzasz wyeksportować swój</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="164"/>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="186"/>
        <source>PRIVATE KEY</source>
        <translation>KLUCZ PRYWATNY</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="165"/>
        <source>This is NOT your Public Key, so &lt;b&gt;DO NOT&lt;/b&gt; share it with anyone.</source>
        <translation>To NIE jest Twój klucz publiczny, więc &lt;b&gt;NIE udostępniaj&lt;/b&gt; go nikomu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="168"/>
        <source>You are exporting a &lt;b&gt;minimum size&lt;/b&gt; private key, which removes all signatures except for the latest self-signatures.</source>
        <translation>Eksportujesz klucz prywatny o &lt;b&gt;minimalnym rozmiarze&lt;/b&gt;, co usuwa wszystkie podpisy poza najnowszymi podpisami własnymi.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="170"/>
        <source>Do you &lt;b&gt;REALLY&lt;/b&gt; want to proceed?</source>
        <translation>Czy &lt;b&gt;NAPRAWDĘ&lt;/b&gt; chcesz kontynuować?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="173"/>
        <source>Exporting Short Private Key</source>
        <translation>Eksportowanie krótkiego klucza prywatnego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="187"/>
        <source>This operation will export your &lt;b&gt;private key&lt;/b&gt;, including both the main key and all subkeys, into an external file. This key is extremely sensitive, and anyone with access to it can impersonate you. DO NOT share this file with anyone!</source>
        <translation>Ta operacja wyeksportuje Twój &lt;b&gt;klucz prywatny&lt;/b&gt;, wraz z kluczem głównym i wszystkimi podkluczami, do pliku zewnętrznego. Ten klucz jest wyjątkowo wrażliwy, a każda osoba mająca do niego dostęp może podszywać się pod Ciebie. NIE udostępniaj tego pliku nikomu!</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="193"/>
        <source>Are you &lt;b&gt;ABSOLUTELY SURE&lt;/b&gt; you want to proceed?</source>
        <translation>Czy masz &lt;b&gt;ABSOLUTNĄ PEWNOŚĆ&lt;/b&gt;, że chcesz kontynuować?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="195"/>
        <source>Exporting Private Key</source>
        <translation>Eksportowanie klucza prywatnego</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ExportKeyPackageDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="54"/>
        <source>Export Key Package</source>
        <translation>Eksportuj pakiet kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="55"/>
        <source>Key Package</source>
        <translation>Pakiet kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="65"/>
        <source>Export Key Package Passphrase</source>
        <translation>Eksportuj hasło pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="67"/>
        <source>Key File</source>
        <translation>Plik kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="98"/>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="143"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="77"/>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="84"/>
        <source>Forbidden</source>
        <translation>Niedozwolone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="78"/>
        <source>Please select an output path before exporting.</source>
        <translation>Przed eksportem wybierz ścieżkę wyjściową.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="85"/>
        <source>Please generate a password to protect your key before exporting, it is very important. Don&apos;t forget to back up your password in a safe place.</source>
        <translation>Przed eksportem wygeneruj hasło chroniące Twój klucz — to bardzo ważne. Nie zapomnij zapisać hasła w bezpiecznym miejscu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="99"/>
        <source>No key is suitable to export.</source>
        <translation>Żaden klucz nie nadaje się do eksportu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="104"/>
        <source>Enter PIN</source>
        <translation>Wprowadź PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="105"/>
        <source>Please enter PIN to protect the Key:</source>
        <translation>Wprowadź PIN chroniący klucz:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="113"/>
        <source>Generating</source>
        <translation>Generowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="123"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="131"/>
        <source>But the key file cannot be leaked under any circumstances. Please delete the Key Package and key file as soon as possible after completing the transfer operation.</source>
        <translation>Ale plik klucza nie może pod żadnym pozorem wyciec. Usuń pakiet kluczy i plik klucza jak najszybciej po zakończeniu operacji przenoszenia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="125"/>
        <source>The Key Package has been successfully generated and has been protected by encryption algorithms(AES-256-GCM). You can safely transfer your Key Package.</source>
        <translation>Pakiet kluczy został pomyślnie wygenerowany i objęty ochroną algorytmów szyfrujących (AES-256-GCM). Możesz bezpiecznie przenosić swój pakiet kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="152"/>
        <source>Key Package Name</source>
        <translation>Nazwa pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="153"/>
        <source>Output Path</source>
        <translation>Ścieżka wyjściowa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="154"/>
        <source>Passphrase</source>
        <translation>Hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="156"/>
        <source>Tips: You can use Key Package to safely and conveniently transfer your public and private keys between devices.</source>
        <translation>Wskazówka: Pakiet kluczy pozwala bezpiecznie i wygodnie przenosić Twoje klucze publiczne i prywatne między urządzeniami.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="158"/>
        <source>Generate and Save Passphrase</source>
        <translation>Wygeneruj i zapisz hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="159"/>
        <source>Generate Key Package Name</source>
        <translation>Wygeneruj nazwę pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="160"/>
        <source>Select Output Path</source>
        <translation>Wybierz ścieżkę wyjściową</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="163"/>
        <source>Include secret key (Think twice before acting)</source>
        <translation>Dołącz klucz prywatny (dobrze się zastanów)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="165"/>
        <source>Exclude keys that do not have a private key</source>
        <translation>Pomiń klucze, które nie mają klucza prywatnego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="168"/>
        <source>Export As Key Package</source>
        <translation>Eksportuj jako pakiet kluczy</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FilePage</name>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="165"/>
        <source>Use ASCII Armor</source>
        <translation>Użyj opakowania ASCII</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="108"/>
        <source>New Folder</source>
        <translation>Nowy folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="109"/>
        <source>Create a new folder in the current folder.</source>
        <translation>Utwórz nowy folder w bieżącym folderze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="116"/>
        <source>New Empty File</source>
        <translation>Nowy pusty plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="117"/>
        <source>Create an empty file in the current folder.</source>
        <translation>Utwórz pusty plik w bieżącym folderze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="130"/>
        <source>Show</source>
        <translation>Pokaż</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="132"/>
        <source>Choose what the file list shows.</source>
        <translation>Wybierz, co pokazuje lista plików.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="135"/>
        <source>Hidden Files</source>
        <translation>Ukryte pliki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="138"/>
        <source>List files and folders whose name starts with a dot.</source>
        <translation>Pokazuj pliki i foldery, których nazwa zaczyna się od kropki.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="143"/>
        <source>System Files</source>
        <translation>Pliki systemowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="146"/>
        <source>List system files such as devices and sockets.</source>
        <translation>Pokazuj pliki systemowe, takie jak urządzenia i gniazda.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="151"/>
        <source>Type Column</source>
        <translation>Kolumna typu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="153"/>
        <source>Show the file type as its own column.</source>
        <translation>Pokazuj typ pliku jako osobną kolumnę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="167"/>
        <source>Write the result of encrypting or signing as printable text (.asc) instead of binary.</source>
        <translation>Zapisz wynik szyfrowania lub podpisywania jako tekst do druku (.asc) zamiast danych binarnych.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="208"/>
        <source>Batch mode is enabled. No file is selected.</source>
        <translation>Tryb wsadowy jest włączony. Nie wybrano żadnego pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="209"/>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="228"/>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="267"/>
        <source>Enable batch mode to select multiple files.</source>
        <translation>Włącz tryb wsadowy, aby wybrać wiele plików.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="214"/>
        <source>%1 item(s) selected.</source>
        <translation>Wybrane elementy: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="227"/>
        <source>Batch mode is enabled. Multiple files can be selected.</source>
        <translation>Tryb wsadowy jest włączony. Można wybrać wiele plików.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="237"/>
        <source>Filter</source>
        <translation>Filtr</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="239"/>
        <source>List only the files and folders whose name contains this text. The filter applies to the current folder and is cleared when you open another one.</source>
        <translation>Pokazuj tylko pliki i foldery, których nazwa zawiera ten tekst. Filtr działa w bieżącym folderze i jest czyszczony po otwarciu innego.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="260"/>
        <source>Go to the parent folder (Backspace)</source>
        <translation>Przejdź do foldera nadrzędnego (Backspace)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="261"/>
        <source>Read this folder from disk again</source>
        <translation>Wczytaj ten folder z dysku ponownie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="263"/>
        <source>Go to a mounted volume or removable drive</source>
        <translation>Przejdź do podłączonego wolumenu lub dysku przenośnego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="265"/>
        <source>Create items and choose what the list shows</source>
        <translation>Twórz elementy i wybierz, co pokazuje lista</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="295"/>
        <source>Entries listed in this folder, and how many of them are selected. Entries hidden by the filter are not counted.</source>
        <translation>Wpisy widoczne w tym folderze i liczba wybranych. Wpisy ukryte przez filtr nie są liczone.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="298"/>
        <source>Space still available on the volume holding this folder.</source>
        <translation>Miejsce jeszcze dostępne na wolumenie zawierającym ten folder.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="433"/>
        <source>Profile Workspace</source>
        <translation>Obszar roboczy profilu</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="481"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n element</numerusform>
            <numerusform>%n elementy</numerusform>
            <numerusform>%n elementów</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="484"/>
        <source>%1 selected</source>
        <translation>Wybrane: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="498"/>
        <source>%1 free</source>
        <translation>Wolna przestrzeń: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="317"/>
        <source>The folder does not exist or cannot be opened.</source>
        <translation>Folder nie istnieje lub nie można go otworzyć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="466"/>
        <source>No Available Volumes</source>
        <translation>Brak dostępnych wolumenów</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FilePathBar</name>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="138"/>
        <source>Type a folder path, e.g. ~/Documents</source>
        <translation>Wpisz ścieżkę folderu, np. ~/Dokumenty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="140"/>
        <source>Type a folder path and press Enter. &quot;~&quot; stands for your home folder, and a relative path is resolved against the current one. Press Escape to go back to the path buttons.</source>
        <translation>Wpisz ścieżkę folderu i naciśnij Enter. „~” oznacza Twój folder domowy, a ścieżka względna jest rozwiązywana względem bieżącej. Naciśnij Escape, aby wrócić do przycisków ścieżki.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="166"/>
        <source>Click a folder in the path to go there, or click the edit button to type a path (Ctrl+L).</source>
        <translation>Kliknij folder w ścieżce, aby tam przejść, albo kliknij przycisk edycji, aby wpisać ścieżkę (Ctrl+L).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="398"/>
        <source>Type a path instead (Ctrl+L)</source>
        <translation>Zamiast tego wpisz ścieżkę (Ctrl+L)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="418"/>
        <source>Show parent folders</source>
        <translation>Pokaż foldery nadrzędne</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FileSystemItemNameDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="68"/>
        <source>Rename Folder</source>
        <translation>Zmień nazwę folderu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="68"/>
        <source>Rename File</source>
        <translation>Zmień nazwę pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="70"/>
        <source>New Folder</source>
        <translation>Nowy folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="70"/>
        <source>New File</source>
        <translation>Nowy plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="81"/>
        <source>Rename this folder</source>
        <translation>Zmień nazwę tego folderu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="81"/>
        <source>Rename this file</source>
        <translation>Zmień nazwę tego pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="84"/>
        <source>Create a new folder</source>
        <translation>Utwórz nowy folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="84"/>
        <source>Create a new empty file</source>
        <translation>Utwórz nowy pusty plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="97"/>
        <source>Location: %1</source>
        <translation>Lokalizacja: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="101"/>
        <source>Folder name</source>
        <translation>Nazwa folderu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="102"/>
        <source>File name, e.g. notes.txt</source>
        <translation>Nazwa pliku, np. notatki.txt</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="109"/>
        <source>Name:</source>
        <translation>Nazwa:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="115"/>
        <source>Create</source>
        <translation>Utwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="115"/>
        <source>Rename</source>
        <translation>Zmień nazwę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="184"/>
        <source>Enter a folder name.</source>
        <translation>Wprowadź nazwę folderu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="184"/>
        <source>Enter a file name.</source>
        <translation>Wprowadź nazwę pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="187"/>
        <source>This name is reserved.</source>
        <translation>Ta nazwa jest zarezerwowana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="190"/>
        <source>The name must not contain path separators.</source>
        <translation>Nazwa nie może zawierać separatorów ścieżki.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="193"/>
        <source>This name is reserved by the operating system.</source>
        <translation>Ta nazwa jest zarezerwowana przez system operacyjny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="196"/>
        <source>Enter a name different from the current one.</source>
        <translation>Wprowadź nazwę inną niż bieżąca.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="199"/>
        <source>A file or folder with this name already exists.</source>
        <translation>Plik lub folder o tej nazwie już istnieje.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="203"/>
        <source>The folder will be renamed in place.</source>
        <translation>Folder zostanie przemianowany w miejscu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="204"/>
        <source>The file will be renamed in place.</source>
        <translation>Plik zostanie przemianowany w miejscu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="210"/>
        <source>The folder will be created in the selected location.</source>
        <translation>Folder zostanie utworzony w wybranej lokalizacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="211"/>
        <source>An empty file will be created in the selected location.</source>
        <translation>Pusty plik zostanie utworzony w wybranej lokalizacji.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FileTreeItemDelegate</name>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="110"/>
        <source>SIG</source>
        <extracomment>Badge on a detached OpenPGP signature file. Keep it to about three characters.</extracomment>
        <translation>SIG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="113"/>
        <source>ENC</source>
        <translation>ENC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="186"/>
        <source>Detached OpenPGP signature</source>
        <translation>Odrębny podpis OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="190"/>
        <source>Encrypted or armored OpenPGP file</source>
        <translation>Zaszyfrowany lub opakowany plik OpenPGP</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FileTreeView</name>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="615"/>
        <source>Rename</source>
        <translation>Zmień nazwę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="599"/>
        <source>Open</source>
        <translation>Otwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="631"/>
        <source>Calculate Hash</source>
        <translation>Oblicz skrót</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="654"/>
        <source>Compress...</source>
        <translation>Kompresuj...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="342"/>
        <source>Unable to Open File</source>
        <translation>Nie można otworzyć pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="266"/>
        <source>This folder cannot be opened</source>
        <translation>Tego folderu nie można otworzyć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="268"/>
        <source>No file matches &quot;%1&quot;</source>
        <translation>Żaden plik nie pasuje do „%1”</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="269"/>
        <source>This folder is empty</source>
        <translation>Ten folder jest pusty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="343"/>
        <source>The file cannot be read.</source>
        <translation>Nie można odczytać pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="353"/>
        <source>Unable to Open Folder</source>
        <translation>Nie można otworzyć folderu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="354"/>
        <source>The folder cannot be opened. Please check permissions.</source>
        <translation>Nie można otworzyć folderu. Sprawdź uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="412"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="622"/>
        <source>Move to Trash</source>
        <translation>Przenieś do kosza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="414"/>
        <source>Move &quot;%1&quot; to Trash?</source>
        <translation>Przenieść „%1” do kosza?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="415"/>
        <source>Move %1 selected items to Trash?</source>
        <translation>Przenieść do kosza wybrane elementy (%1)?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="437"/>
        <source>Unable to Move to Trash</source>
        <translation>Nie można przenieść do kosza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="439"/>
        <source>The item &quot;%1&quot; could not be moved to Trash.</source>
        <translation>Nie udało się przenieść elementu „%1” do kosza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="441"/>
        <source>%1 item(s) could not be moved to Trash.</source>
        <translation>Nie udało się przenieść do kosza elementów: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="446"/>
        <source>Moved %1 item(s) to Trash.</source>
        <translation>Przeniesiono do kosza: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="469"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="477"/>
        <source>Unable to Create Folder</source>
        <translation>Nie można utworzyć folderu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="470"/>
        <source>The target folder is not available.</source>
        <translation>Folder docelowy jest niedostępny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="478"/>
        <source>The folder could not be created. Please check permissions.</source>
        <translation>Nie udało się utworzyć folderu. Sprawdź uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="486"/>
        <source>Created folder: %1</source>
        <translation>Utworzono folder: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="507"/>
        <source>Unable to Create File</source>
        <translation>Nie można utworzyć pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="508"/>
        <source>The file could not be created. Please check permissions.</source>
        <translation>Nie udało się utworzyć pliku. Sprawdź uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="522"/>
        <source>Created file: %1</source>
        <translation>Utworzono plik: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="579"/>
        <source>Unable to Rename</source>
        <translation>Nie można zmienić nazwy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="580"/>
        <source>The file or folder could not be renamed.</source>
        <translation>Nie udało się zmienić nazwy pliku lub folderu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="586"/>
        <source>Renamed &quot;%1&quot; to &quot;%2&quot;.</source>
        <translation>Zmieniono nazwę „%1” na „%2”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="601"/>
        <source>Open the file in a GpgFrontend editor tab (Enter).</source>
        <translation>Otwórz plik w karcie edytora GpgFrontend (Enter).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="608"/>
        <source>Open with Default Application</source>
        <translation>Otwórz w domyślnej aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="610"/>
        <source>Hand the item to the application your system uses for it.</source>
        <translation>Przekaż element aplikacji, której system używa w tym celu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="616"/>
        <source>Give the item a new name (F2).</source>
        <translation>Nadaj elementowi nową nazwę (F2).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="624"/>
        <source>Move the selected items to the system Trash, where they can still be recovered (Delete).</source>
        <translation>Przenieś wybrane elementy do systemowego kosza, skąd nadal można je odzyskać (Delete).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="633"/>
        <source>Compute checksums of the file and show them on the information board, to compare it against a published value.</source>
        <translation>Oblicz sumy kontrolne pliku i pokaż je na tablicy informacji, aby porównać je z opublikowaną wartością.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="639"/>
        <source>Folder</source>
        <translation>Folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="641"/>
        <source>Create a new folder inside the selected folder.</source>
        <translation>Utwórz nowy folder w wybranym folderze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="646"/>
        <source>Empty File</source>
        <translation>Pusty plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="648"/>
        <source>Create an empty file inside the selected folder.</source>
        <translation>Utwórz pusty plik w wybranym folderze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="658"/>
        <source>Copy Path</source>
        <translation>Skopiuj ścieżkę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="660"/>
        <source>Copy the full path of the selected items to the clipboard.</source>
        <translation>Skopiuj pełną ścieżkę wybranych elementów do schowka.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="665"/>
        <source>Refresh</source>
        <translation>Odśwież</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="666"/>
        <source>Read this folder from disk again.</source>
        <translation>Wczytaj ten folder z dysku ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="670"/>
        <source>New</source>
        <translation>Nowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="673"/>
        <source>Create a new item in the current folder.</source>
        <translation>Utwórz nowy element w bieżącym folderze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="782"/>
        <source>Calculating</source>
        <translation>Obliczanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="801"/>
        <source>File Hash Information</source>
        <translation>Informacje o skrócie pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="851"/>
        <source>Batch mode is enabled. Use Ctrl or Shift to select multiple items.</source>
        <translation>Tryb wsadowy jest włączony. Użyj Ctrl lub Shift, aby wybrać wiele elementów.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1014"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1022"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1056"/>
        <source>Move Failed</source>
        <translation>Przenoszenie nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1015"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1212"/>
        <source>A file or folder named &quot;%1&quot; already exists in the target folder.</source>
        <translation>Plik lub folder o nazwie „%1” już istnieje w folderze docelowym.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1023"/>
        <source>Unable to move &quot;%1&quot;.

The target may be on another volume, or you may not have sufficient permissions.</source>
        <translation>Nie można przenieść „%1”.

Cel może znajdować się na innym wolumenie lub możesz nie mieć wystarczających uprawnień.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1056"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1211"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1225"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1234"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1249"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1257"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1282"/>
        <source>Copy Failed</source>
        <translation>Kopiowanie nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1057"/>
        <source>The target folder is not writable.</source>
        <translation>Do folderu docelowego nie można zapisywać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1080"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1103"/>
        <source>The source and target folder are the same.</source>
        <translation>Folder źródłowy i docelowy są tym samym folderem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1104"/>
        <source>Cannot put &quot;%1&quot; into itself or one of its subfolders.</source>
        <translation>Nie można umieścić „%1” w nim samym ani w jednym z jego podfolderów.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1116"/>
        <source>Move Items</source>
        <translation>Przenieś elementy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1119"/>
        <source>Move &quot;%1&quot; to &quot;%2&quot;?</source>
        <translation>Przenieść „%1” do „%2”?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1121"/>
        <source>Move %1 items to &quot;%2&quot;?</source>
        <translation>Przenieść elementy (%1) do „%2”?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1125"/>
        <source>Copy Items</source>
        <translation>Skopiuj elementy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1128"/>
        <source>Copy &quot;%1&quot; to &quot;%2&quot;?</source>
        <translation>Skopiować „%1” do „%2”?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1130"/>
        <source>Copy %1 items to &quot;%2&quot;?</source>
        <translation>Skopiować elementy (%1) do „%2”?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1135"/>
        <source>Move</source>
        <translation>Przenieś</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1135"/>
        <source>Copy</source>
        <translation>Skopiuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1177"/>
        <source>Moved %1 item(s).</source>
        <translation>Przeniesiono elementy: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1178"/>
        <source>Copied %1 item(s).</source>
        <translation>Skopiowano elementy: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1184"/>
        <source>Move Partially Completed</source>
        <translation>Przenoszenie zakończone częściowo</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1185"/>
        <source>Copy Partially Completed</source>
        <translation>Kopiowanie zakończone częściowo</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1187"/>
        <source>Some items could not be moved. Please check permissions or whether the target is on another volume.</source>
        <translation>Niektórych elementów nie udało się przenieść. Sprawdź uprawnienia lub to, czy cel znajduje się na innym wolumenie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1189"/>
        <source>Some items could not be copied. Please check permissions.</source>
        <translation>Niektórych elementów nie udało się skopiować. Sprawdź uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1226"/>
        <source>Unable to copy &quot;%1&quot;. Please check permissions.</source>
        <translation>Nie można skopiować „%1”. Sprawdź uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1235"/>
        <source>&quot;%1&quot; is not a regular file or folder.</source>
        <translation>„%1” nie jest zwykłym plikiem ani folderem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1250"/>
        <source>The target folder &quot;%1&quot; already exists.</source>
        <translation>Folder docelowy „%1” już istnieje.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1258"/>
        <source>Unable to create target folder &quot;%1&quot;.</source>
        <translation>Nie można utworzyć folderu docelowego „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1283"/>
        <source>Unable to copy &quot;%1&quot;.</source>
        <translation>Nie można skopiować „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1303"/>
        <source>Path copied to clipboard.</source>
        <translation>Ścieżka skopiowana do schowka.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1304"/>
        <source>%1 paths copied to clipboard.</source>
        <translation>Skopiowano do schowka ścieżki: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1315"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1333"/>
        <source>File list refreshed.</source>
        <translation>Lista plików odświeżona.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1323"/>
        <source>Unable to Refresh</source>
        <translation>Nie można odświeżyć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1324"/>
        <source>The current folder no longer exists or cannot be opened.</source>
        <translation>Bieżący folder już nie istnieje lub nie można go otworzyć.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FindWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/FindWidget.cpp" line="47"/>
        <source>Find</source>
        <translation>Znajdź</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GRTTreeView</name>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="103"/>
        <source>No runtime values published yet.</source>
        <translation>Nie opublikowano jeszcze żadnych wartości.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="119"/>
        <source>Copy Key Path</source>
        <translation>Skopiuj ścieżkę klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="125"/>
        <source>Copy Value</source>
        <translation>Skopiuj wartość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="131"/>
        <source>Expand All</source>
        <translation>Rozwiń wszystkie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="132"/>
        <source>Collapse All</source>
        <translation>Zwiń wszystkie</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GeneralTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="52"/>
        <source>Base</source>
        <translation>Podstawy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="59"/>
        <source>Module Loading Policy:</source>
        <translation>Zasady ładowania modułów:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="60"/>
        <source>Only Integrated Modules</source>
        <translation>Tylko moduły wbudowane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="62"/>
        <source>All Modules</source>
        <translation>Wszystkie moduły</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="63"/>
        <source>Disable</source>
        <translation>Wyłącz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="67"/>
        <source>Default Workspace As:</source>
        <translation>Domyślna przestrzeń robocza:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="68"/>
        <source>File Panel</source>
        <translation>Panel plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="69"/>
        <source>Text Editor</source>
        <translation>Edytor tekstu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="78"/>
        <source>Cache text editor contents.</source>
        <translation>Przechowuj zawartość edytora tekstu w pamięci podręcznej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="99"/>
        <source>System Default follows your operating system. Choose a language here to override it. GpgFrontend restarts automatically when you change it.</source>
        <translation>Domyślny systemowy to język zgodny z ustawionym w systemie operacyjnym. Wybierz tutaj język, aby to zmienić. GpgFrontend uruchomi się ponownie automatycznie po zmianie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="65"/>
        <source>Operation</source>
        <translation>Zachowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="54"/>
        <source>Default Engine:</source>
        <translation>Domyślny silnik:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="75"/>
        <source>File Panel opens at</source>
        <translation>Panel plików otwiera się w</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="81"/>
        <source>Import files dropped on the Key List without confirmation.</source>
        <translation>Importuj pliki upuszczone na listę kluczy bez potwierdzania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="84"/>
        <source>Remember which keys are checked in the key list between restarts.</source>
        <translation>Zapamiętuj zaznaczone klucze na liście kluczy między uruchomieniami.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="86"/>
        <source>Use Binary Mode for File Operations</source>
        <translation>Używaj trybu binarnego dla operacji na plikach</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="88"/>
        <source>Warn about keys expiring within:</source>
        <translation>Ostrzegaj o kluczach wygasających w ciągu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="89"/>
        <source> days</source>
        <translation> dni</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="91"/>
        <source>Language</source>
        <translation>Język</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="98"/>
        <source>NOTE</source>
        <translation>UWAGA</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="98"/>
        <source>: </source>
        <translation>: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="103"/>
        <source>Data</source>
        <translation>Dane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="105"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="142"/>
        <source>Clear All Data Objects (Total Size: %1)</source>
        <translation>Wyczyść wszystkie obiekty danych (całkowity rozmiar: %1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="121"/>
        <source>Reveal in File Explorer</source>
        <translation>Pokaż w eksploratorze plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="135"/>
        <source>Confirm</source>
        <translation>Potwierdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="136"/>
        <source>Are you sure you want to clear all data objects?
This will result in loss of all cached form positions, statuses, key servers, etc.</source>
        <translation>Czy na pewno chcesz wyczyścić wszystkie obiekty danych?
Spowoduje to utratę wszystkich zapamiętanych pozycji okien, statusów, serwerów kluczy itd.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="177"/>
        <source>Profile Workspace</source>
        <translation>Obszar roboczy profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="183"/>
        <source>Home Folder</source>
        <translation>Folder domowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="186"/>
        <source>Working Directory</source>
        <translation>Katalog roboczy</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GenerateCardKeyDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="53"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="54"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="55"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="56"/>
        <source>Expire Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="57"/>
        <source>Non Expire</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="59"/>
        <source>Warning: This Operation will overwrite keys which exist on card!</source>
        <translation>Ostrzeżenie: ta operacja nadpisze klucze istniejące na karcie!</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="60"/>
        <source>Generate</source>
        <translation>Wygeneruj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="77"/>
        <source>Card Key Generation</source>
        <translation>Generowanie kluczy karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="96"/>
        <source>Name must not be empty.</source>
        <translation>Nazwa nie może być pusta.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="102"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Nazwa i komentarz nie mogą zawierać znaków &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; ani znaków kontrolnych.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="108"/>
        <source>Please give a valid email address.</source>
        <translation>Podaj prawidłowy adres e-mail.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="151"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="152"/>
        <source>Card key generation completed successfully.</source>
        <translation>Generowanie kluczy karty zakończone pomyślnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="158"/>
        <source>Generating</source>
        <translation>Generowanie</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GenerateRevocationCert</name>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="44"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Bez powodu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="44"/>
        <source>1 -&gt; This key is no more safe.</source>
        <translation>1 -&gt; Ten klucz nie jest już bezpieczny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="45"/>
        <source>2 -&gt; Key is outdated.</source>
        <translation>2 -&gt; Klucz jest przestarzały.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="45"/>
        <source>3 -&gt; Key is no longer used</source>
        <translation>3 -&gt; Klucz nie jest już używany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="53"/>
        <source>Revocation Certificates</source>
        <translation>Certyfikaty unieważnienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="65"/>
        <source>Generate revocation certificate</source>
        <translation>Wygeneruj certyfikat unieważnienia</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GnuPGTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="49"/>
        <source>General</source>
        <translation>Ogólne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="50"/>
        <source>Advanced</source>
        <translation>Zaawansowane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="52"/>
        <source>Enable GpgME Debug Log</source>
        <translation>Włącz dziennik debugowania GpgME</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="54"/>
        <source>Enable verbose GpgME logs for troubleshooting. This may include technical details about GnuPG operations.</source>
        <translation>Włącz szczegółowe dzienniki GpgME do rozwiązywania problemów. Mogą zawierać techniczne szczegóły operacji GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="57"/>
        <source>Use Custom GnuPG</source>
        <translation>Używaj własnego GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="58"/>
        <source>Select GnuPG Path</source>
        <translation>Wybierz ścieżkę GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="60"/>
        <source>Terminate GnuPG background processes on exit</source>
        <translation>Kończ procesy GnuPG w tle przy wyjściu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="62"/>
        <source>This may affect other applications that are using GnuPG.</source>
        <translation>Może to wpłynąć na inne aplikacje korzystające z GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="64"/>
        <source>Forbid all GnuPG network connection.</source>
        <translation>Zabroń wszystkich połączeń sieciowych GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="69"/>
        <source>Clear password cache on exit</source>
        <translation>Czyść pamięć podręczną haseł przy wyjściu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="71"/>
        <source>Ask gpg-agent to forget all cached passphrases when GpgFrontend closes.</source>
        <translation>Poproś gpg-agenta o zapomnienie wszystkich zapamiętanych haseł po zamknięciu GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="77"/>
        <source>Select the directory that contains the &quot;gpgconf&quot; executable.</source>
        <translation>Wybierz katalog zawierający plik wykonywalny „gpgconf”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="105"/>
        <source>Open Directory</source>
        <translation>Otwórz katalog</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="159"/>
        <source>Maintenance</source>
        <translation>Konserwacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="163"/>
        <source>These operations take effect immediately and are not undone by cancelling this dialog. Restarting components briefly interrupts any in-flight GnuPG operation.</source>
        <translation>Te operacje działają natychmiast i anulowanie tego okna ich nie cofa. Ponowne uruchomienie składników krótko przerywa każdą trwającą operację GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="170"/>
        <source>Clear Password Cache</source>
        <translation>Wyczyść pamięć podręczną haseł</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="171"/>
        <source>Clear Password Cache of GnuPG</source>
        <translation>Wyczyść pamięć podręczną haseł GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="175"/>
        <source>Clear password cache successfully</source>
        <translation>Pomyślnie wyczyszczono pamięć podręczną haseł</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="176"/>
        <source>Failed to clear password cache of GnuPG</source>
        <translation>Nie udało się wyczyścić pamięci podręcznej haseł GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="180"/>
        <source>Reload Components</source>
        <translation>Przeładuj składniki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="181"/>
        <source>Reload All GnuPG&apos;s Components</source>
        <translation>Przeładuj wszystkie składniki GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="185"/>
        <source>Reload all the GnuPG&apos;s components successfully</source>
        <translation>Pomyślnie przeładowano wszystkie składniki GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="186"/>
        <source>Failed to reload all or one of the GnuPG&apos;s component(s)</source>
        <translation>Nie udało się przeładować wszystkich lub jednego ze składników GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="190"/>
        <source>Restart Components</source>
        <translation>Uruchom ponownie składniki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="191"/>
        <source>Restart All GnuPG&apos;s Components</source>
        <translation>Uruchom ponownie wszystkie składniki GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="195"/>
        <source>Confirm</source>
        <translation>Potwierdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="196"/>
        <source>Are you sure you want to restart all of GnuPG&apos;s components?
Any GnuPG operation still running will be interrupted.</source>
        <translation>Czy na pewno chcesz ponownie uruchomić wszystkie składniki GnuPG?
Każda trwająca operacja GnuPG zostanie przerwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="203"/>
        <source>Restart all the GnuPG&apos;s components successfully</source>
        <translation>Pomyślnie ponownie uruchomiono wszystkie składniki GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="204"/>
        <source>Failed to restart all or one of the GnuPG&apos;s component(s)</source>
        <translation>Nie udało się ponownie uruchomić wszystkich lub jednego ze składników GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="228"/>
        <source>GnuPG Unavailable</source>
        <translation>GnuPG niedostępne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="232"/>
        <source>GnuPG cannot start: this key database&apos;s folder path is too long for GnuPG&apos;s agent socket. Choose a key database in a shorter path under Settings, Key Databases.</source>
        <translation>GnuPG nie może wystartować: ścieżka folderu tej bazy kluczy jest za długa dla gniazda agenta GnuPG. Wybierz bazę kluczy o krótszej ścieżce w Ustawieniach, w sekcji Bazy kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="273"/>
        <source>Successful Operation</source>
        <translation>Operacja zakończona pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="275"/>
        <source>Failed Operation</source>
        <translation>Operacja nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="373"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="380"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="394"/>
        <source>Illegal GnuPG Path</source>
        <translation>Nieprawidłowa ścieżka GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="374"/>
        <source>Target GnuPG Path is not an exists readable directory.</source>
        <translation>Docelowa ścieżka GnuPG nie jest istniejącym, odczytywalnym katalogiem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="381"/>
        <source>Target GnuPG Path is not an absolute path.</source>
        <translation>Docelowa ścieżka GnuPG nie jest ścieżką bezwzględną.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="395"/>
        <source>Target GnuPG Path contains no &quot;gpgconf&quot; executable.</source>
        <translation>Docelowa ścieżka GnuPG nie zawiera pliku wykonywalnego „gpgconf”.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GpgFrontendApplication</name>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="80"/>
        <source>Standard Exception Thrown</source>
        <translation>Wystąpił standardowy wyjątek</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="81"/>
        <source>Oops, an standard exception was thrown during the running of the program. This is not a serious problem, it may be the negligence of the programmer, please report this problem if you can.</source>
        <translation>Ups, podczas działania programu wystąpił standardowy wyjątek. To nie jest poważny problem; może to być przeoczenie programisty — zgłoś ten problem, jeśli możesz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="89"/>
        <source>Unhandled Exception Thrown</source>
        <translation>Wystąpił nieobsłużony wyjątek</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="90"/>
        <source>Oops, an unhandled exception was thrown during the running of the program. This is not a serious problem, it may be the negligence of the programmer, please report this problem if you can.</source>
        <translation>Ups, podczas działania programu wystąpił nieobsłużony wyjątek. To nie jest poważny problem; może to być przeoczenie programisty — zgłoś ten problem, jeśli możesz.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GpgOperaHelper</name>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="302"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="372"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="446"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="512"/>
        <source>Operation Not Supported</source>
        <translation>Operacja nieobsługiwana</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="311"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="381"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="454"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="520"/>
        <source>Operation Cancelled</source>
        <translation>Operacja anulowana</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="321"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="391"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="463"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="529"/>
        <source>Critical Error</source>
        <translation>Błąd krytyczny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="759"/>
        <source>Another operation is still running. Please wait for it to finish.</source>
        <translation>Inna operacja wciąż trwa. Poczekaj na jej zakończenie.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InfoBoardWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="73"/>
        <source>Copy</source>
        <translation>Skopiuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="74"/>
        <source>Save</source>
        <translation>Zapisz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="75"/>
        <source>Magnify</source>
        <translation>Powiększ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="76"/>
        <source>Clear</source>
        <translation>Wyczyść</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="78"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="46"/>
        <source>Copy status text</source>
        <translation>Skopiuj tekst statusu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="79"/>
        <source>Save status text to file</source>
        <translation>Zapisz tekst statusu do pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="80"/>
        <source>Magnify the generated document</source>
        <translation>Powiększ wygenerowany dokument</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="81"/>
        <source>Clear status panel</source>
        <translation>Wyczyść panel statusu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="128"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="564"/>
        <source>Operation status will appear here.</source>
        <translation>Status operacji pojawi się tutaj.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="195"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="196"/>
        <source>Details</source>
        <translation>Szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="197"/>
        <source>Show the summary report</source>
        <translation>Pokaż raport podsumowujący</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="198"/>
        <source>Show the raw status text</source>
        <translation>Pokaż surowy tekst statusu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="302"/>
        <source>The operation completed successfully.</source>
        <translation>Operacja zakończyła się pomyślnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="303"/>
        <source>%1 completed successfully.</source>
        <translation>%1 zakończyło się pomyślnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="306"/>
        <source>Completed with warnings — please review the details.</source>
        <translation>Zakończono z ostrzeżeniami — przejrzyj szczegóły.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="307"/>
        <source>%1 completed with warnings — please review the details.</source>
        <translation>%1 zakończyło się z ostrzeżeniami — przejrzyj szczegóły.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="311"/>
        <source>The operation failed. See the details for more information.</source>
        <translation>Operacja nie powiodła się. Więcej informacji znajdziesz w szczegółach.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="313"/>
        <source>%1 failed. See the details for more information.</source>
        <translation>%1 nie powiodło się. Więcej informacji znajdziesz w szczegółach.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="323"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="325"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="469"/>
        <source>Warning</source>
        <translation>Ostrzeżenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="327"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="330"/>
        <source>Information</source>
        <translation>Informacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="339"/>
        <source>The operation finished and everything checked out.</source>
        <translation>Operacja zakończona i wszystko w porządku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="341"/>
        <source>The operation finished, but something needs your attention. Read the details below.</source>
        <translation>Operacja zakończona, ale coś wymaga Twojej uwagi. Przeczytaj szczegóły poniżej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="345"/>
        <source>The operation failed, or the result cannot be trusted. Read the details below.</source>
        <translation>Operacja nie powiodła się lub wynikowi nie można ufać. Przeczytaj szczegóły poniżej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="350"/>
        <source>Nothing has been checked yet. The light turns green, orange or red once an operation finishes.</source>
        <translation>Nic jeszcze nie zostało sprawdzone. Kontrolka zmieni kolor na zielony, pomarańczowy lub czerwony po zakończeniu operacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="368"/>
        <source>Green</source>
        <translation>Zielony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="368"/>
        <source>Everything went well</source>
        <translation>Wszystko przebiegło pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="369"/>
        <source>Orange</source>
        <translation>Pomarańczowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="370"/>
        <source>Done, but check the details</source>
        <translation>Gotowe, ale sprawdź szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="371"/>
        <source>Red</source>
        <translation>Czerwony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="372"/>
        <source>Failed, or the result cannot be trusted</source>
        <translation>Niepowodzenie lub wynikowi nie można ufać</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="373"/>
        <source>Grey</source>
        <translation>Szary</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="374"/>
        <source>Idle, nothing checked yet</source>
        <translation>Bezczynność, nic jeszcze nie sprawdzono</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="405"/>
        <source>[%1] No details available.</source>
        <translation>[%1] Brak dostępnych szczegółów.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="538"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="235"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="105"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="210"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="237"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="295"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="416"/>
        <source>DETAILS</source>
        <translation>SZCZEGÓŁY</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="51"/>
        <source>GPGFRONTEND SECURITY REPORT</source>
        <translation>RAPORT BEZPIECZEŃSTWA GPGFRONTEND</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="147"/>
        <source>No operation yet.
Results will appear here as a summary document.</source>
        <translation>Nie wykonano jeszcze żadnej operacji.
Wyniki pojawią się tutaj jako podsumowanie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="224"/>
        <source>OPERATION</source>
        <translation>OPERACJA</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="225"/>
        <source>STATUS</source>
        <translation>STATUS</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="226"/>
        <source>ENGINE</source>
        <translation>SILNIK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="44"/>
        <source>Copied</source>
        <translation>Skopiowano</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="85"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="132"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="166"/>
        <source>Unable to Save</source>
        <translation>Nie można zapisać</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="86"/>
        <source>The content could not be captured.</source>
        <translation>Nie udało się przechwycić zawartości.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="92"/>
        <source>GpgFrontend Security Report</source>
        <translation>Raport bezpieczeństwa GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="94"/>
        <source>Cryptographic operation result report</source>
        <translation>Raport z wyniku operacji kryptograficznej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="104"/>
        <source>Hash: </source>
        <translation>Skrót: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="133"/>
        <source>The image could not be saved.</source>
        <translation>Nie udało się zapisać obrazu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="145"/>
        <source>Export Certificate</source>
        <translation>Eksportuj certyfikat</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="146"/>
        <source>PNG Image (*.png);;All Files (*)</source>
        <translation>Obraz PNG (*.png);;Wszystkie pliki (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="159"/>
        <source>Save Status Panel Content</source>
        <translation>Zapisz zawartość panelu statusu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="160"/>
        <source>Text Files (*.txt);;All Files (*)</source>
        <translation>Pliki tekstowe (*.txt);;Wszystkie pliki (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="167"/>
        <source>The file could not be saved. Please check the path and permissions.</source>
        <translation>Nie udało się zapisać pliku. Sprawdź ścieżkę i uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="141"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="557"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="588"/>
        <source>Issued:  %1</source>
        <translation>Wystawiono:  %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="185"/>
        <source>Hash: %1</source>
        <translation>Skrót: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="190"/>
        <source>Export certificate as PNG image</source>
        <translation>Eksportuj certyfikat jako obraz PNG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="203"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="343"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="567"/>
        <source>Operation: %1</source>
        <translation>Operacja: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="205"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="345"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="569"/>
        <source>Status:    %1</source>
        <translation>Status:    %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="206"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="348"/>
        <source>SHA-256:   %1</source>
        <translation>SHA-256:   %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="231"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="411"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="631"/>
        <source>Decrypt</source>
        <translation>Odszyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="232"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="412"/>
        <source>RECIPIENT</source>
        <translation>ODBIORCA</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="233"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="413"/>
        <source>Sign</source>
        <translation>Podpisanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="234"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="413"/>
        <source>Verify</source>
        <translation>Weryfikacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="235"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="414"/>
        <source>SIGNER</source>
        <translation>PODPISUJĄCY</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="346"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="570"/>
        <source>Engine:    %1</source>
        <translation>Silnik:    %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="352"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="574"/>
        <source>Details:   %1</source>
        <translation>Szczegóły:   %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="466"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="472"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="503"/>
        <source>… and %1 more</source>
        <translation>… oraz %1 więcej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="579"/>
        <source>  SHA-256: %1</source>
        <translation>  SHA-256: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="616"/>
        <source>  Signer: %1</source>
        <translation>  Podpisujący: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="618"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="625"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="635"/>
        <source>  Key ID: %1</source>
        <translation>  Identyfikator klucza: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="623"/>
        <source>  Signed: %1</source>
        <translation>  Podpisano: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="628"/>
        <source>  Invalid signer: %1 — %2</source>
        <translation>  Nieprawidłowy podpisujący: %1 — %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="630"/>
        <source>Encrypt</source>
        <translation>Szyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="634"/>
        <source>  Recipient: %1</source>
        <translation>  Odbiorca: %1</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InfoTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="290"/>
        <source>Developer</source>
        <translation>Deweloper</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="266"/>
        <source>A user-friendly OpenPGP tool for encryption, signing, and key management.</source>
        <translation>Przyjazne dla użytkownika narzędzie OpenPGP do szyfrowania, podpisywania i zarządzania kluczami.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="285"/>
        <source>Developed and maintained by Saturneric.</source>
        <translation>Tworzone i utrzymywane przez Saturneric.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="286"/>
        <source>Report an issue on GitHub</source>
        <translation>Zgłoś problem na GitHubie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="287"/>
        <source>About and contact information</source>
        <translation>Informacje o programie i kontakt</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="296"/>
        <source>Website:</source>
        <translation>Strona internetowa:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="299"/>
        <source>Documentation:</source>
        <translation>Dokumentacja:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="300"/>
        <source>User guides and overview</source>
        <translation>Przewodniki użytkownika i przegląd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="302"/>
        <source>Source code:</source>
        <translation>Kod źródłowy:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="305"/>
        <source>Release notes:</source>
        <translation>Informacje o wydaniu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="306"/>
        <source>Changelog and downloads</source>
        <translation>Dziennik zmian i pliki do pobrania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="311"/>
        <source>Resources</source>
        <translation>Zasoby</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="318"/>
        <source>GpgFrontend is free software, licensed under &lt;a href=&quot;https://www.gnu.org/licenses/gpl-3.0.html&quot;&gt;GPL-3.0-or-later&lt;/a&gt;.</source>
        <translation>GpgFrontend jest wolnym oprogramowaniem na licencji &lt;a href=&quot;https://www.gnu.org/licenses/gpl-3.0.html&quot;&gt;GPL-3.0-or-later&lt;/a&gt;.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InstantMessagingTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="71"/>
        <source>Message Book Phrase</source>
        <translation>Fraza Książki Wiadomości</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="75"/>
        <source>A long secret you share with one friend. It makes your messages look like random text, so nobody can tell they are PGP at all. You and your friend must use exactly the same phrase.</source>
        <translation>Długi sekret, który dzielisz z jednym znajomym. Sprawia, że Twoje wiadomości wyglądają jak losowy tekst, więc nikt nie pozna, że to PGP. Ty i Twój znajomy musicie używać dokładnie tej samej frazy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="88"/>
        <source>No phrase set. Messages use the built-in default book.</source>
        <translation>Nie ustawiono frazy. Wiadomości używają wbudowanej domyślnej książki.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="104"/>
        <source>Generate</source>
        <translation>Wygeneruj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="106"/>
        <source>Create a new random phrase. Share it with your friend so you both use the same one.</source>
        <translation>Utwórz nową losową frazę. Przekaż ją znajomemu, żebyście używali tej samej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="116"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="221"/>
        <source>Show</source>
        <translation>Pokaż</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="117"/>
        <source>Show or hide the phrase.</source>
        <translation>Pokaż lub ukryj frazę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="121"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="172"/>
        <source>Copy</source>
        <translation>Skopiuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="122"/>
        <source>Copy the phrase to the clipboard.</source>
        <translation>Skopiuj frazę do schowka.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="126"/>
        <source>Paste</source>
        <translation>Wklej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="128"/>
        <source>Replace the phrase with the one on the clipboard.</source>
        <translation>Zastąp frazę tą ze schowka.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="134"/>
        <source>Clear</source>
        <translation>Wyczyść</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="136"/>
        <source>Remove the phrase and fall back to the default book.</source>
        <translation>Usuń frazę i wróć do domyślnej książki.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="154"/>
        <source>Book Fingerprint</source>
        <translation>Odcisk książki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="158"/>
        <source>A short code made from your phrase. Read it out with your friend to be sure you both have the same one. Unlike the phrase, this code is safe to say out loud.</source>
        <translation>Krótki kod wygenerowany z Twojej frazy. Odczytaj go znajomemu, aby upewnić się, że macie ten sam. W przeciwieństwie do frazy, ten kod można bezpiecznie wypowiedzieć na głos.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="173"/>
        <source>Copy the fingerprint to the clipboard.</source>
        <translation>Skopiuj odcisk do schowka.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="185"/>
        <source>The phrase is stored in the encrypted cache, never in the settings file. Send it to your friend over a private channel.</source>
        <translation>Fraza jest przechowywana w zaszyfrowanej pamięci podręcznej, nigdy w pliku ustawień. Prześlij ją znajomemu prywatnym kanałem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="221"/>
        <source>Hide</source>
        <translation>Ukryj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="241"/>
        <source>No phrase set. Using the built-in default.</source>
        <translation>Nie ustawiono frazy. Używana jest wbudowana domyślna książka.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="242"/>
        <source>Phrase set. %1 characters.</source>
        <translation>Fraza ustawiona. Znaków: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="252"/>
        <source>Calculating…</source>
        <translation>Obliczanie…</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::IntroPage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="318"/>
        <source>Welcome to GpgFrontend</source>
        <translation>Witamy w GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="319"/>
        <source>Let&apos;s get you started in just a moment.</source>
        <translation>Zaraz zaczynamy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="322"/>
        <source>A simple, privacy-focused OpenPGP tool for text, files, and keys.</source>
        <translation>Proste narzędzie OpenPGP dbające o prywatność do pracy z tekstem, plikami i kluczami.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="325"/>
        <source>Language:</source>
        <translation>Język:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="326"/>
        <source>System Default</source>
        <translation>Domyślny systemowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="329"/>
        <source>&lt;b&gt;GpgFrontend&lt;/b&gt; helps you encrypt, decrypt, sign, and verify messages and files with OpenPGP. This short wizard will point you to the most useful places to start.</source>
        <translation>&lt;b&gt;GpgFrontend&lt;/b&gt; pomaga szyfrować, odszyfrowywać, podpisywać i weryfikować wiadomości oraz pliki w standardzie OpenPGP. Ten krótki kreator wskaże najbardziej przydatne miejsca na start.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="348"/>
        <source>You can change update checking, key database, and appearance settings later from the application settings.</source>
        <translation>Sprawdzanie aktualizacji, bazę kluczy i ustawienia wyglądu możesz później zmienić w ustawieniach aplikacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="334"/>
        <source>Star GpgFrontend on GitHub</source>
        <translation>Daj gwiazdkę GpgFrontend na GitHubie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="335"/>
        <source>GpgFrontend is free and open source. A star helps more people discover it and keeps the project moving forward.</source>
        <translation>GpgFrontend jest darmowy i otwartoźródłowy. Gwiazdka pomaga większej liczbie osób go poznać i utrzymuje projekt w rozwoju.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="339"/>
        <source>Open the overview page</source>
        <translation>Otwórz stronę przeglądu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="340"/>
        <source>Get a quick tour of the main features and common workflows.</source>
        <translation>Zapoznaj się szybko z głównymi funkcjami i podstawowymi działaniami.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="343"/>
        <source>Fundamental concepts</source>
        <translation>Podstawowe pojęcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="344"/>
        <source>Understand public keys, private keys, encryption, signing, and trust.</source>
        <translation>Zrozum klucze publiczne, klucze prywatne, szyfrowanie, podpisywanie i zaufanie.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyActionState</name>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="44"/>
        <source>Select a key first.</source>
        <translation>Najpierw wybierz klucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="49"/>
        <source>Select exactly one key for this.</source>
        <translation>Do tego wybierz dokładnie jeden klucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="54"/>
        <source>Only available for a key, not a key group.</source>
        <translation>Dostępne tylko dla klucza, nie dla grupy kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="60"/>
        <source>Only available for a key you own the private half of.</source>
        <translation>Dostępne tylko dla klucza, którego połowę prywatną posiadasz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="66"/>
        <source>Check or select at least one key first.</source>
        <translation>Najpierw zaznacz lub wybierz co najmniej jeden klucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="139"/>
        <source>This key carries no email address.</source>
        <translation>Ten klucz nie ma adresu e-mail.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="176"/>
        <source>Certifying is for vouching for someone else&apos;s key.</source>
        <translation>Poświadczanie służy do poręczania za cudzy klucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="198"/>
        <source>Tick the box beside at least one key first.</source>
        <translation>Najpierw zaznacz pole obok co najmniej jednego klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="234"/>
        <source>Expiry can only be changed on keys you own.</source>
        <translation>Datę wygaśnięcia można zmieniać tylko we własnych kluczach.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="245"/>
        <source>This keyring holds no private keys.</source>
        <translation>Ta baza kluczy nie zawiera kluczy prywatnych.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDatabaseEditDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="135"/>
        <source>Convert to Relative Path</source>
        <translation>Zamień na ścieżkę względną</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="131"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="133"/>
        <source>Engine</source>
        <translation>Silnik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="134"/>
        <source>Choose Folder…</source>
        <translation>Wybierz folder…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="132"/>
        <source>Folder</source>
        <translation>Folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="136"/>
        <source>e.g. Personal Keys</source>
        <translation>np. Klucze osobiste</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="154"/>
        <source>Open Directory</source>
        <translation>Otwórz katalog</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="166"/>
        <source>Illegal GnuPG Key Database Path</source>
        <translation>Nieprawidłowa ścieżka bazy kluczy GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="167"/>
        <source>Target GnuPG Key Database Path is not an exists readable directory.</source>
        <translation>Docelowa ścieżka bazy kluczy GnuPG nie jest istniejącym, odczytywalnym katalogiem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="223"/>
        <source>Add Key Database</source>
        <translation>Dodaj bazę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="224"/>
        <source>New key database in your profile</source>
        <translation>Nowa baza kluczy w Twoim profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="226"/>
        <source>GpgFrontend picks the folder and keeps it inside your profile, so this key database travels with it. Just choose a name.</source>
        <translation>GpgFrontend sam wybiera folder i przechowuje go wewnątrz Twojego profilu, więc ta baza kluczy podróżuje razem z nim. Podaj tylko nazwę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="231"/>
        <source>Rename Key Database</source>
        <translation>Zmień nazwę bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="232"/>
        <source>Rename this key database</source>
        <translation>Zmień nazwę tej bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="233"/>
        <source>The folder it is kept in is named after it, and is renamed too.</source>
        <translation>Folder, w którym jest przechowywana, nosi jej nazwę, więc również zostanie przemianowany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="238"/>
        <source>Add Key Database On This Computer</source>
        <translation>Dodaj bazę kluczy na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="239"/>
        <source>New key database on this computer</source>
        <translation>Nowa baza kluczy na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="241"/>
        <source>Choose a folder yourself. A key database outside your profile belongs to this computer alone, and a profile package never carries it.</source>
        <translation>Wybierz folder samodzielnie. Baza kluczy poza Twoim profilem należy wyłącznie do tego komputera i pakiet profilu nigdy jej nie przenosi.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="247"/>
        <source>Edit Key Database</source>
        <translation>Edytuj bazę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="248"/>
        <source>Key database on this computer</source>
        <translation>Baza kluczy na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="250"/>
        <source>This key database belongs to this computer alone, and a profile package never carries it.</source>
        <translation>Ta baza kluczy należy wyłącznie do tego komputera i pakiet profilu nigdy jej nie przenosi.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="301"/>
        <source>Choose a name first</source>
        <translation>Najpierw wybierz nazwę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="302"/>
        <source>No folder chosen yet</source>
        <translation>Nie wybrano jeszcze folderu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="345"/>
        <source>The key database name cannot be empty.</source>
        <translation>Nazwa bazy kluczy nie może być pusta.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="354"/>
        <source>&quot;%1&quot; is the name of the key database GpgFrontend derives from your OpenPGP engine. Turn that one on with the checkbox in Key Database settings, or pick another name.</source>
        <translation>„%1” to nazwa bazy kluczy, którą GpgFrontend wyznacza na podstawie Twojego silnika OpenPGP. Włącz ją, zaznaczając odpowiednie pole w ustawieniach baz kluczy, albo wybierz inną nazwę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="363"/>
        <source>The key database path cannot be empty.</source>
        <translation>Ścieżka bazy kluczy nie może być pusta.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="374"/>
        <source>A key database with the name &apos;%1&apos; already exists. Please choose a different name.</source>
        <translation>Baza kluczy o nazwie „%1” już istnieje. Wybierz inną nazwę.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDatabasesTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="295"/>
        <source>Unreadable Key Database Settings</source>
        <translation>Nieodczytywalne ustawienia baz kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="296"/>
        <source>Your saved key database list exists but could not be read. This usually means it was written by another installation, or with a different application key.</source>
        <translation>Twoja zapisana lista baz kluczy istnieje, ale nie udało się jej odczytać. Zwykle oznacza to, że została zapisana przez inną instalację lub przy użyciu innego klucza aplikacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="300"/>
        <source>Saving now replaces it with the list shown here. The previous list cannot be recovered afterwards.</source>
        <translation>Zapisanie teraz zastąpi ją listą pokazaną tutaj. Poprzedniej listy nie będzie można później odzyskać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="594"/>
        <source>No Key Database Selected</source>
        <translation>Nie wybrano bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="595"/>
        <source>Please select a key database to edit.</source>
        <translation>Wybierz bazę kluczy do edycji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="655"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="705"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="796"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="805"/>
        <source>Duplicate Key Database Paths</source>
        <translation>Zduplikowane ścieżki baz kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="656"/>
        <source>The edited key database path duplicates a previously existing one.</source>
        <translation>Edytowana ścieżka bazy kluczy duplikuje już istniejącą.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="644"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="735"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="787"/>
        <source>Invalid Key Database Paths</source>
        <translation>Nieprawidłowe ścieżki baz kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="190"/>
        <source>Use this computer&apos;s default key database</source>
        <translation>Użyj domyślnej bazy kluczy tego komputera</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="191"/>
        <source>Add Key Database</source>
        <translation>Dodaj bazę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="193"/>
        <source>Add Key Database On This Computer</source>
        <translation>Dodaj bazę kluczy na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="357"/>
        <source>The OpenPGP engine on this computer does not report a key database, so there is no default one to use.</source>
        <translation>Silnik OpenPGP na tym komputerze nie zgłasza żadnej bazy kluczy, więc nie ma domyślnej bazy, której można by użyć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="386"/>
        <source>Always available on this computer, and always channel 0.</source>
        <translation>Zawsze dostępna na tym komputerze i zawsze jako kanał 0.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="387"/>
        <source>Always channel 0. Never included in a profile package: whoever opens your profile elsewhere gets their own.</source>
        <translation>Zawsze jako kanał 0. Nigdy nie jest uwzględniana w pakiecie profilu: kto otworzy Twój profil gdzie indziej, dostanie własną.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="437"/>
        <source>This folder could not be resolved on this computer.</source>
        <translation>Nie udało się ustalić tego folderu na tym komputerze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="473"/>
        <source>No key databases of your own yet. One you add here is kept inside your profile and travels with it.</source>
        <translation>Nie masz jeszcze własnych baz kluczy. Baza dodana tutaj jest przechowywana wewnątrz Twojego profilu i podróżuje razem z nim.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="475"/>
        <source>Kept inside your profile, and carried by a profile package.</source>
        <translation>Przechowywana wewnątrz Twojego profilu i przenoszona przez pakiet profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="479"/>
        <source>Nothing here yet. A key database you add on this tab stays on this computer: a profile package never carries it, and it is always opened after the ones inside your profile.</source>
        <translation>Nic tu jeszcze nie ma. Baza kluczy dodana na tej karcie pozostaje na tym komputerze: pakiet profilu nigdy jej nie przenosi, a otwierana jest zawsze po bazach znajdujących się w Twoim profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="482"/>
        <source>Key databases outside your profile. They belong to this computer alone: a profile package never carries them, and they are always opened after the ones inside your profile.</source>
        <translation>Bazy kluczy poza Twoim profilem. Należą wyłącznie do tego komputera: pakiet profilu nigdy ich nie przenosi, a otwierane są zawsze po bazach znajdujących się w Twoim profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="544"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="690"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="767"/>
        <source>Currently, GpgFrontend supports a maximum of %1 key databases. Please remove an existing database to add a new one.</source>
        <translation>Obecnie GpgFrontend obsługuje maksymalnie %1 baz kluczy. Usuń istniejącą bazę, aby dodać nową.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="574"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="846"/>
        <source>Key Database Required</source>
        <translation>Wymagana baza kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="575"/>
        <source>GpgFrontend needs at least one key database. Add another one first, then turn this off.</source>
        <translation>GpgFrontend potrzebuje co najmniej jednej bazy kluczy. Najpierw dodaj inną, a dopiero potem wyłącz tę opcję.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="622"/>
        <source>Key Database Folder Already Exists</source>
        <translation>Folder bazy kluczy już istnieje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="623"/>
        <source>A folder named after &apos;%1&apos; is already there. Its contents are not ours to replace, so choose another name.</source>
        <translation>Folder o nazwie „%1” już tam istnieje. Nie możemy zastąpić jego zawartości, więc wybierz inną nazwę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="633"/>
        <source>Failed to Rename Key Database Folder</source>
        <translation>Nie udało się zmienić nazwy folderu bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="634"/>
        <source>GpgFrontend could not rename the folder this key database is kept in. It may still be in use; try again after restarting GpgFrontend.</source>
        <translation>GpgFrontend nie mógł zmienić nazwy folderu, w którym przechowywana jest ta baza kluczy. Może on być nadal w użyciu; spróbuj ponownie po ponownym uruchomieniu GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="645"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="736"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="788"/>
        <source>The edited key database path is not a valid path that GpgFrontend can use</source>
        <translation>Edytowana ścieżka bazy kluczy nie jest prawidłową ścieżką, której GpgFrontend może użyć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="543"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="689"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="766"/>
        <source>Maximum Key Database Limit Reached</source>
        <translation>Osiągnięto limit baz kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="706"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="797"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="806"/>
        <source>The newly added key database path duplicates a previously existing one.</source>
        <translation>Nowo dodana ścieżka bazy kluczy duplikuje już istniejącą.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="715"/>
        <source>Invalid Key Database Path</source>
        <translation>Nieprawidłowa ścieżka bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="716"/>
        <source>The specified key database path points to an existing file. Please specify a path that does not exist or points to a directory.</source>
        <translation>Podana ścieżka bazy kluczy wskazuje na istniejący plik. Podaj ścieżkę, która nie istnieje lub wskazuje na katalog.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="725"/>
        <source>Failed to Create Key Database Directory</source>
        <translation>Nie udało się utworzyć katalogu bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="726"/>
        <source>GpgFrontend failed to create a directory at the specified key database path. Please check the path and your permissions.</source>
        <translation>GpgFrontend nie mógł utworzyć katalogu pod podaną ścieżką bazy kluczy. Sprawdź ścieżkę i swoje uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="847"/>
        <source>GpgFrontend needs at least one key database, so the last one cannot be removed.</source>
        <translation>GpgFrontend potrzebuje co najmniej jednej bazy kluczy, więc nie można usunąć ostatniej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="853"/>
        <source>Confirm Deletion</source>
        <translation>Potwierdź usunięcie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="854"/>
        <source>Are you sure you want to remove the selected key database from the list?</source>
        <translation>Czy na pewno chcesz usunąć wybraną bazę kluczy z listy?</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDetailsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="53"/>
        <source>KeyPair</source>
        <translation>Para kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="66"/>
        <source>UIDs</source>
        <translation>Identyfikatory użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="72"/>
        <source>Photo IDs</source>
        <translation>Identyfikatory zdjęć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="77"/>
        <source>Keychain</source>
        <translation>Pęk kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="80"/>
        <source>Operations</source>
        <translation>Operacje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="98"/>
        <source>Key Details</source>
        <translation>Szczegóły klucza</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGenerateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1393"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1488"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1494"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1591"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1760"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1798"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="167"/>
        <source>Custom</source>
        <translation>Własny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="173"/>
        <source>3 Months</source>
        <translation>3 miesiące</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="176"/>
        <source>6 Months</source>
        <translation>6 miesięcy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="179"/>
        <source>1 Year</source>
        <translation>1 rok</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="163"/>
        <source>2 Years</source>
        <translation>2 lata</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="183"/>
        <source>5 Years</source>
        <translation>5 lat</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="186"/>
        <source>10 Years</source>
        <translation>10 lat</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="529"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="540"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="189"/>
        <source>Non Expired</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="484"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="877"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1499"/>
        <source>Primary Key Only</source>
        <translation>Tylko klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="485"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="940"/>
        <source>Primary Key With Subkey</source>
        <translation>Klucz główny z podkluczem</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="491"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="500"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="492"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="493"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="499"/>
        <source>Key Database</source>
        <translation>Baza kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="513"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="531"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="501"/>
        <source>Combination</source>
        <translation>Kombinacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="488"/>
        <source>Profile</source>
        <translation>Profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="502"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="514"/>
        <source>Validity Period</source>
        <translation>Okres ważności</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1746"/>
        <source>Save Profile</source>
        <translation>Zapisz profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="506"/>
        <source>Save current configuration as a new profile</source>
        <translation>Zapisz bieżącą konfigurację jako nowy profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1817"/>
        <source>Delete Profile</source>
        <translation>Usuń profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="508"/>
        <source>Delete current selected profile</source>
        <translation>Usuń aktualnie wybrany profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="489"/>
        <source>Basic</source>
        <translation>Podstawowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="504"/>
        <source>Save</source>
        <translation>Zapisz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="507"/>
        <source>Delete</source>
        <translation>Usuń</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="721"/>
        <source>Name must not be empty.</source>
        <translation>Nazwa nie może być pusta.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1844"/>
        <source>Reset To Default</source>
        <translation>Przywróć domyślne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="511"/>
        <source>Reset profile list to default configuration</source>
        <translation>Przywróć listę profili do domyślnej konfiguracji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="497"/>
        <source>Fill in a random anonymous identity (for throwaway or test keys)</source>
        <translation>Wypełnij losową anonimową tożsamością (dla kluczy jednorazowych lub testowych)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="509"/>
        <source>Reset</source>
        <translation>Resetuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="515"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="533"/>
        <source>Key Length</source>
        <translation>Długość klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="516"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="535"/>
        <source>Second Algorithm</source>
        <translation>Drugi algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="517"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="536"/>
        <source>Second Key Length</source>
        <translation>Druga długość klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="518"/>
        <source>Key Format</source>
        <translation>Format klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="519"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="534"/>
        <source>Usage</source>
        <translation>Zastosowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="523"/>
        <source>v4 (Compatible)</source>
        <translation>v4 (kompatybilny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="524"/>
        <source>v6 (Modern)</source>
        <translation>v6 (nowoczesny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="525"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="537"/>
        <source>Encrypt</source>
        <translation>Szyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="526"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="538"/>
        <source>Sign</source>
        <translation>Podpisywanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="527"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="539"/>
        <source>Authentication</source>
        <translation>Uwierzytelnianie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="528"/>
        <source>No Passphrase</source>
        <translation>Bez hasła</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="532"/>
        <source>Expire Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="543"/>
        <source>Easy Mode</source>
        <translation>Tryb łatwy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="544"/>
        <source>Primary Key</source>
        <translation>Klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="545"/>
        <source>Subkey</source>
        <translation>Podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="548"/>
        <source>Generate</source>
        <translation>Wygeneruj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="565"/>
        <source>%1 Engine Version: %2</source>
        <translation>Wersja silnika %1: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="570"/>
        <source>If subkey is specified, it will be generated together with the primary key. Therefore, you may need to enter the passphrase additionally for the subkey generation.</source>
        <translation>Jeśli podano podklucz, zostanie on wygenerowany razem z kluczem głównym. W związku z tym może być konieczne dodatkowe wprowadzenie hasła podczas generowania podklucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="576"/>
        <source>Supported Primary Key Algorithms: </source>
        <translation>Obsługiwane algorytmy klucza głównego: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="586"/>
        <source>Supported Subkey Algorithms: </source>
        <translation>Obsługiwane algorytmy podkluczy: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="596"/>
        <source>Please select a key algorithm and configure the parameters as needed.</source>
        <translation>Wybierz algorytm klucza i skonfiguruj parametry według potrzeb.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="606"/>
        <source>Generate Key</source>
        <translation>Wygeneruj klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="727"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Nazwa i komentarz nie mogą zawierać znaków &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; ani znaków kontrolnych.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="733"/>
        <source>Please give a valid email address.</source>
        <translation>Podaj prawidłowy adres e-mail.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="738"/>
        <source>Please give a valid primary key algorithm.</source>
        <translation>Podaj prawidłowy algorytm klucza głównego.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="747"/>
        <source>Time to primary key expiration must not be less than 120 seconds.</source>
        <translation>Czas do wygaśnięcia klucza głównego nie może być mniejszy niż 120 sekund.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="754"/>
        <source>Please give a valid subkey algorithm.</source>
        <translation>Podaj prawidłowy algorytm podklucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="762"/>
        <source>Time to subkey expiration must not be less than 120 seconds.</source>
        <translation>Czas do wygaśnięcia podklucza nie może być mniejszy niż 120 sekund.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="792"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="793"/>
        <source>Key generation completed successfully.</source>
        <translation>Generowanie klucza zakończone pomyślnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1524"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1525"/>
        <source>Unknown error occurred</source>
        <translation>Wystąpił nieznany błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1540"/>
        <source>Generating</source>
        <translation>Generowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1668"/>
        <source>ECC</source>
        <translation>ECC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1673"/>
        <source>Post-Quantum</source>
        <translation>Pokwantowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1747"/>
        <source>Please enter profile name:</source>
        <translation>Podaj nazwę profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1755"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1761"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1769"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1783"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1799"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1811"/>
        <source>Notice</source>
        <translation>Uwaga</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1756"/>
        <source>Profile was not saved: Name cannot be empty.</source>
        <translation>Profil nie został zapisany: nazwa nie może być pusta.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1762"/>
        <source>The profile name &apos;Custom&apos; is reserved. Please choose another name.</source>
        <translation>Nazwa profilu „Własny” jest zarezerwowana. Wybierz inną nazwę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1770"/>
        <source>Profile was not saved: Name cannot be longer than %1 characters.</source>
        <translation>Profil nie został zapisany: nazwa nie może być dłuższa niż %1 znaków.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1784"/>
        <source>Profile was not saved: Name already exists.</source>
        <translation>Profil nie został zapisany: nazwa już istnieje.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1800"/>
        <source>The &apos;Custom&apos; profile cannot be deleted.</source>
        <translation>Nie można usunąć profilu „Własny”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1812"/>
        <source>Selected profile does not exist.</source>
        <translation>Wybrany profil nie istnieje.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1818"/>
        <source>Are you sure you want to delete the profile &apos;%1&apos;?</source>
        <translation>Czy na pewno chcesz usunąć profil „%1”?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1845"/>
        <source>Are you sure you want to reset the easy profile configuration to default? This action cannot be undone.</source>
        <translation>Czy na pewno chcesz przywrócić domyślną konfigurację profili trybu łatwego? Tej operacji nie można cofnąć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="2012"/>
        <source>Post-quantum algorithms require the v6 key format.</source>
        <translation>Algorytmy pokwantowe wymagają formatu klucza v6.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupCreationDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="100"/>
        <source>Create</source>
        <translation>Utwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="55"/>
        <source>Engineering Team</source>
        <translation>Zespół inżynierów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="58"/>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="61"/>
        <source>Optional</source>
        <translation>Opcjonalne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="63"/>
        <source>Create a Key Group</source>
        <translation>Utwórz grupę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="69"/>
        <source>Encrypting to a key group encrypts to every key it contains.</source>
        <translation>Szyfrowanie do grupy kluczy szyfruje do każdego klucza, który zawiera.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="89"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="91"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="93"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="101"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="49"/>
        <source>New Key Group</source>
        <translation>Nowa grupa kluczy</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupEditDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="48"/>
        <source>Edit Key Group</source>
        <translation>Edytuj grupę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="56"/>
        <source>Edit Key Group Details</source>
        <translation>Edytuj szczegóły grupy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="62"/>
        <source>These details only name the group. Changing them does not affect which keys belong to it.</source>
        <translation>Te dane tylko nazywają grupę. Ich zmiana nie wpływa na to, które klucze do niej należą.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="79"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="81"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="83"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="90"/>
        <source>Save</source>
        <translation>Zapisz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="91"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupManageDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="95"/>
        <source>Key Group Management</source>
        <translation>Zarządzanie grupami kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="125"/>
        <source>Encrypting to this group encrypts to every key it contains, including the members of any group nested inside it.</source>
        <translation>Szyfrowanie do tej grupy szyfruje do każdego klucza, który zawiera, w tym do członków każdej grupy zagnieżdżonej w niej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="135"/>
        <source>Close</source>
        <translation>Zamknij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="174"/>
        <source>Edit…</source>
        <translation>Edytuj…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="176"/>
        <source>Change the name, email and comment of this key group.</source>
        <translation>Zmień nazwę, e-mail i komentarz tej grupy kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="182"/>
        <source>Delete Group…</source>
        <translation>Usuń grupę…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="184"/>
        <source>Delete this key group. The keys in it are not touched.</source>
        <translation>Usuń tę grupę kluczy. Klucze w niej pozostają bez zmian.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="208"/>
        <source>Filter members</source>
        <translation>Filtruj członków</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="236"/>
        <source>This group has no members yet.

Check keys on the right and press Add.</source>
        <translation>Ta grupa nie ma jeszcze członków.

Zaznacz klucze po prawej i naciśnij Dodaj.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="264"/>
        <source>Members</source>
        <translation>Członkowie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="279"/>
        <source>Add</source>
        <translation>Dodaj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="282"/>
        <source>Add the keys checked on the right to this group.</source>
        <translation>Dodaj klucze zaznaczone po prawej do tej grupy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="287"/>
        <source>Remove</source>
        <translation>Usuń</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="290"/>
        <source>Remove the members checked on the left from this group.</source>
        <translation>Usuń członków zaznaczonych po lewej z tej grupy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="319"/>
        <source>Available</source>
        <translation>Dostępne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="342"/>
        <source>Every key that can be added is already in this group.</source>
        <translation>Każdy klucz, który można dodać, jest już w tej grupie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="350"/>
        <source>Available Keys</source>
        <translation>Dostępne klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="417"/>
        <source>created %1</source>
        <translation>utworzono: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="471"/>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="492"/>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="617"/>
        <source>Some Keys Failed</source>
        <translation>Problem z niektórymi kluczami</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="472"/>
        <source>Some keys could not be added to the group:
%1</source>
        <translation>Niektórych kluczy nie udało się dodać do grupy:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="493"/>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="618"/>
        <source>Some keys could not be removed from the group:
%1</source>
        <translation>Niektórych kluczy nie udało się usunąć z grupy:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="528"/>
        <source>Update Failed</source>
        <translation>Aktualizacja nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="529"/>
        <source>This key group could not be updated.</source>
        <translation>Nie udało się zaktualizować tej grupy kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="554"/>
        <source>Delete Key Group</source>
        <translation>Usuń grupę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="564"/>
        <source>Delete Failed</source>
        <translation>Usuwanie nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="565"/>
        <source>This key group could not be deleted.</source>
        <translation>Nie udało się usunąć tej grupy kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="609"/>
        <source>Remove from Group</source>
        <translation>Usuń z grupy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="624"/>
        <source>Manage Group…</source>
        <translation>Zarządzaj grupą…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="629"/>
        <source>Key Details…</source>
        <translation>Szczegóły klucza…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="652"/>
        <source>This Key Group contains some invalid keys:

%1

These keys are no longer available. Do you want to remove them from the group?</source>
        <translation>Ta grupa kluczy zawiera kilka nieprawidłowych kluczy:

%1

Te klucze nie są już dostępne. Czy chcesz usunąć je z grupy?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="658"/>
        <source>Invalid Keys in Group</source>
        <translation>Nieprawidłowe klucze w grupie</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupMetadataRules</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="68"/>
        <source>Name must contain at least five characters.</source>
        <translation>Nazwa musi zawierać co najmniej pięć znaków.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="73"/>
        <source>Email does not look like an address. Leave it empty if the group has none.</source>
        <translation>E-mail nie wygląda na adres. Zostaw puste, jeśli grupa go nie ma.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="85"/>
        <source>This group is empty.</source>
        <translation>Ta grupa jest pusta.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="95"/>
        <source>1 key</source>
        <translation>1 klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="97"/>
        <source>%1 keys</source>
        <translation>kluczy: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="102"/>
        <source>1 nested group</source>
        <translation>1 grupa zagnieżdżona</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="105"/>
        <source>%1 nested groups</source>
        <translation>grup zagnieżdżonych: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="111"/>
        <source> and </source>
        <translation> i </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="115"/>
        <source>Contains %1.</source>
        <translation>Zawiera %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="121"/>
        <source>1 member is no longer in your keyring.</source>
        <translation>1 członek nie znajduje się już w Twojej bazie kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="124"/>
        <source>%1 members are no longer in your keyring.</source>
        <translation>%1 członków nie znajduje się już w Twojej bazie kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="137"/>
        <source>1 checked key will be added to this group.</source>
        <translation>1 zaznaczony klucz zostanie dodany do tej grupy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="140"/>
        <source>%1 checked keys will be added to this group.</source>
        <translation>Do tej grupy zostanie dodanych zaznaczonych kluczy: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="148"/>
        <source>Delete the key group &quot;%1&quot;?

The group is removed. The keys in it are not touched and stay in your keyring.</source>
        <translation>Usunąć grupę kluczy „%1”?

Grupa zostanie usunięta. Zawarte w niej klucze nie zostaną zmienione i pozostaną w Twojej bazie kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="156"/>
        <source>It is also a member of: %1. Those groups will lose it.</source>
        <translation>Jest ona również członkiem: %1. Tamte grupy ją stracą.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyImportDetailDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="45"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="66"/>
        <source>Key Import Details</source>
        <translation>Szczegóły importu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="46"/>
        <source>No keys found to import</source>
        <translation>Nie znaleziono kluczy do zaimportowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="161"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="175"/>
        <source>Considered</source>
        <translation>Rozważone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="162"/>
        <source>Imported</source>
        <translation>Zaimportowane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="164"/>
        <source>Not Imported</source>
        <translation>Niezaimportowane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="168"/>
        <source>New Revocations</source>
        <translation>Nowe unieważnienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="214"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="214"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="214"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="215"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="259"/>
        <source>Private</source>
        <translation>Prywatny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="262"/>
        <source>Public</source>
        <translation>Publiczny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="163"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="265"/>
        <source>Unchanged</source>
        <translation>Bez zmian</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="103"/>
        <source>Import Completed with Issues</source>
        <translation>Import zakończony z problemami</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="104"/>
        <source>%1 of %2 key(s) could not be imported.</source>
        <translation>Nie udało się zaimportować %1 z %2 kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="109"/>
        <source>Import Successful</source>
        <translation>Import zakończony pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="111"/>
        <source>Successfully processed %1 key(s).</source>
        <translation>Pomyślnie przetworzono kluczy: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="113"/>
        <source>Nothing to Import</source>
        <translation>Nic do zaimportowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="115"/>
        <source>All %1 key(s) are already up to date.</source>
        <translation>Wszystkie klucze (%1) są już aktualne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="143"/>
        <source>Summary</source>
        <translation>Podsumowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="165"/>
        <source>Private Keys Read</source>
        <translation>Odczytane klucze prywatne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="166"/>
        <source>Private Keys Imported</source>
        <translation>Zaimportowane klucze prywatne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="167"/>
        <source>Private Keys Unchanged</source>
        <translation>Niezmienione klucze prywatne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="268"/>
        <source>New Key</source>
        <translation>Nowy klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="271"/>
        <source>New Subkey</source>
        <translation>Nowy podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="275"/>
        <source>New Signature</source>
        <translation>Nowy podpis</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="279"/>
        <source>New UID</source>
        <translation>Nowy identyfikator użytkownika</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyList</name>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="603"/>
        <source>Refresh</source>
        <translation>Odśwież</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="605"/>
        <source>Refresh the key list to synchronize changes.</source>
        <translation>Odśwież listę kluczy, aby zsynchronizować zmiany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="607"/>
        <source>Sync Public Key</source>
        <translation>Synchronizuj klucz publiczny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="745"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="755"/>
        <source>Subkeys</source>
        <translation>Podklucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="763"/>
        <source>No optional columns</source>
        <translation>Brak kolumn opcjonalnych</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1446"/>
        <source>Refreshing Key List...</source>
        <translation>Odświeżanie listy kluczy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1526"/>
        <source>Import Keys</source>
        <translation>Importuj klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1451"/>
        <source>Key List Refreshed.</source>
        <translation>Listę kluczy odświeżono.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="747"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="340"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1315"/>
        <source>New Category...</source>
        <translation>Nowa kategoria...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="609"/>
        <source>Sync public keys with the key server configured as the default.</source>
        <translation>Zsynchronizuj klucze publiczne z serwerem kluczy ustawionym jako domyślny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="611"/>
        <source>Uncheck All</source>
        <translation>Odznacz wszystkie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="612"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="927"/>
        <source>Uncheck all keys in the current tab.</source>
        <translation>Odznacz wszystkie klucze w bieżącej karcie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="614"/>
        <source>Check All</source>
        <translation>Zaznacz wszystkie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="615"/>
        <source>Check all keys in the current tab.</source>
        <translation>Zaznacz wszystkie klucze w bieżącej karcie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="618"/>
        <source>Search keys by user ID, key ID, fingerprint...</source>
        <translation>Szukaj kluczy według identyfikatora użytkownika, identyfikatora klucza, odcisku klucza...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="620"/>
        <source>Columns</source>
        <translation>Kolumny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="621"/>
        <source>Choose visible key table columns.</source>
        <translation>Wybierz widoczne kolumny tabeli kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="625"/>
        <source>Create a key group from checked encryption-capable keys.</source>
        <translation>Utwórz grupę kluczy z zaznaczonych kluczy zdolnych do szyfrowania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="717"/>
        <source>Switch between key databases.</source>
        <translation>Przełączaj między bazami kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="657"/>
        <source>Channel %1</source>
        <translation>Kanał %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="698"/>
        <source>No key database available</source>
        <translation>Brak dostępnej bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="707"/>
        <source>Key Database</source>
        <translation>Baza kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="718"/>
        <source>Click to switch key databases.</source>
        <translation>Kliknij, aby przełączyć między bazami kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="751"/>
        <source>Expire Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="753"/>
        <source>Owner Trust</source>
        <translation>Zaufanie do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="759"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="768"/>
        <source>Reset Column Widths</source>
        <translation>Przywróć szerokości kolumn</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="929"/>
        <source>%n key(s) checked</source>
        <translation>
            <numerusform>Zaznaczono %n klucz</numerusform>
            <numerusform>Zaznaczono %n klucze</numerusform>
            <numerusform>Zaznaczono %n kluczy</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="937"/>
        <source>%n of them are not shown by the current category or search filter.</source>
        <translation>
            <numerusform>%n z nich nie jest widoczny w bieżącej kategorii lub filtrze wyszukiwania.</numerusform>
            <numerusform>%n z nich nie są widoczne w bieżącej kategorii lub filtrze wyszukiwania.</numerusform>
            <numerusform>%n z nich nie jest widocznych w bieżącej kategorii lub filtrze wyszukiwania.</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1254"/>
        <source>Delete Category</source>
        <translation>Usuń kategorię</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1255"/>
        <source>Delete category &quot;%1&quot;? This removes the grouping only; the keys themselves are not affected.</source>
        <translation>Usunąć kategorię „%1”? Usuwane jest tylko grupowanie; same klucze nie są dotykane.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1321"/>
        <source>Set Colour...</source>
        <translation>Ustaw kolor...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1325"/>
        <source>Reset Colour</source>
        <translation>Przywróć kolor</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1336"/>
        <source>Rename Category...</source>
        <translation>Zmień nazwę kategorii...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1340"/>
        <source>Delete Category...</source>
        <translation>Usuń kategorię...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1354"/>
        <source>Choose Category Colour</source>
        <translation>Wybierz kolor kategorii</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1390"/>
        <source>New Category</source>
        <translation>Nowa kategoria</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1390"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1409"/>
        <source>Category name:</source>
        <translation>Nazwa kategorii:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1396"/>
        <source>Category Colour (optional)</source>
        <translation>Kolor kategorii (opcjonalnie)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1409"/>
        <source>Rename Category</source>
        <translation>Zmień nazwę kategorii</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1528"/>
        <source>You&apos;ve dropped something on the key list.
GpgFrontend will now try to import key(s).</source>
        <translation>Upuszczono coś na listę kluczy.
GpgFrontend spróbuje teraz zaimportować klucz(-e).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1533"/>
        <source>Ask before importing keys next time.</source>
        <translation>Pytaj przed importem kluczy następnym razem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1796"/>
        <source>No key matches &quot;%1&quot;</source>
        <translation>Żaden klucz nie pasuje do „%1”</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1797"/>
        <source>%n key(s) match &quot;%1&quot;</source>
        <translation>
            <numerusform>%n klucz pasuje do „%1”</numerusform>
            <numerusform>%n klucze pasują do „%1”</numerusform>
            <numerusform>%n kluczy pasuje do „%1”</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="749"/>
        <source>Create Date</source>
        <translation>Data utworzenia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="757"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="623"/>
        <source>New Key Group</source>
        <translation>Nowa grupa kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1684"/>
        <source>The key has been updated</source>
        <translation>Klucz został zaktualizowany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1686"/>
        <source>No need to update the key</source>
        <translation>Nie trzeba aktualizować klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1722"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1750"/>
        <source>Syncing Key List...</source>
        <translation>Synchronizowanie listy kluczy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1727"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1755"/>
        <source>Sync [%1/%2] %3 %4</source>
        <translation>Synchronizacja [%1/%2] %3 %4</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1737"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1765"/>
        <source>Key List Sync Done.</source>
        <translation>Synchronizacja listy kluczy zakończona.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyMgmt</name>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="82"/>
        <source>All</source>
        <translation>Wszystkie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="87"/>
        <source>Key Group</source>
        <translation>Grupa kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="93"/>
        <source>Only Public Key</source>
        <translation>Klucze publiczne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="101"/>
        <source>Has Private Key</source>
        <translation>Klucze prywatne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="109"/>
        <source>Expiring Soon</source>
        <translation>Wkrótce wygasną</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="115"/>
        <source>No Primary Key</source>
        <translation>Bez klucza głównego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="125"/>
        <source>Revoked</source>
        <translation>Unieważnione</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="132"/>
        <source>Expired</source>
        <translation>Wygasłe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="139"/>
        <source>Disabled</source>
        <translation>Wyłączone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="188"/>
        <source>KeyPair Management</source>
        <translation>Zarządzanie parami kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="245"/>
        <source>Copy</source>
        <translation>Skopiuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="255"/>
        <source>Export</source>
        <translation>Eksportuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="263"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="751"/>
        <source>Key Operations</source>
        <translation>Operacje na kluczach</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="274"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="709"/>
        <source>Keyserver</source>
        <translation>Serwer kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="280"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1603"/>
        <source>Category</source>
        <translation>Kategoria</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="341"/>
        <source>Open</source>
        <translation>Otwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="341"/>
        <source>Open Key File</source>
        <translation>Otwórz plik klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="347"/>
        <source>Close</source>
        <translation>Zamknij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="352"/>
        <source>New Keypair</source>
        <translation>Nowa para kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="353"/>
        <source>Generate KeyPair</source>
        <translation>Wygeneruj parę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="360"/>
        <source>New Subkey</source>
        <translation>Nowy podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="361"/>
        <source>Generate Subkey For Selected KeyPair</source>
        <translation>Wygeneruj podklucz dla wybranej pary kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="370"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="648"/>
        <source>File</source>
        <translation>Plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="371"/>
        <source>Import New Key From File</source>
        <translation>Importuj nowy klucz z pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="377"/>
        <source>Clipboard</source>
        <translation>Schowek</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="378"/>
        <source>Import New Key From Clipboard</source>
        <translation>Importuj nowy klucz ze schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="386"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1289"/>
        <source>Key Package</source>
        <translation>Pakiet kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="387"/>
        <source>Import Key(s) From a Key Package</source>
        <translation>Importuj klucze z pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="392"/>
        <source>Export To Clipboard</source>
        <translation>Eksportuj do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="393"/>
        <source>Export Checked Key(s) To Clipboard</source>
        <translation>Eksportuj zaznaczone klucze do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="398"/>
        <source>Export As Key Package</source>
        <translation>Eksportuj jako pakiet kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="399"/>
        <source>Export Checked Key(s) To a Key Package</source>
        <translation>Eksportuj zaznaczone klucze do pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="404"/>
        <source>Export As OpenSSH</source>
        <translation>Eksportuj jako OpenSSH</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="414"/>
        <source>Delete the Selected keys</source>
        <translation>Usuń wybrane klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="405"/>
        <source>Export a single key in OpenSSH format to a file</source>
        <translation>Wyeksportuj pojedynczy klucz w formacie OpenSSH do pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="413"/>
        <source>Delete Selected Keys</source>
        <translation>Usuń wybrane klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="420"/>
        <source>Refresh Key List</source>
        <translation>Odśwież listę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="421"/>
        <source>Re-read the keyring from disk</source>
        <translation>Wczytaj ponownie bazę kluczy z dysku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="429"/>
        <source>Find Key</source>
        <translation>Znajdź klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="429"/>
        <source>Jump to the search box</source>
        <translation>Przejdź do pola wyszukiwania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="436"/>
        <source>Delete Checked Keys</source>
        <translation>Usuń zaznaczone klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="437"/>
        <source>Delete the Checked keys</source>
        <translation>Usuń zaznaczone klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="442"/>
        <source>Show Key Details</source>
        <translation>Pokaż szczegóły klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="443"/>
        <source>Show Details for this Key</source>
        <translation>Pokaż szczegóły tego klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="448"/>
        <source>Set Owner Trust Level</source>
        <translation>Ustaw poziom zaufania do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="449"/>
        <source>Set how much you trust this key to certify others</source>
        <translation>Ustaw, jak bardzo ufasz temu kluczowi przy poświadczaniu innych</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="494"/>
        <source>Copy Fingerprint</source>
        <translation>Skopiuj odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="495"/>
        <source>Copy this key&apos;s full fingerprint to the clipboard</source>
        <translation>Skopiuj pełny odcisk tego klucza do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="499"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="502"/>
        <source>Copy Key ID</source>
        <translation>Skopiuj identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="503"/>
        <source>Copy this key&apos;s ID to the clipboard</source>
        <translation>Skopiuj identyfikator tego klucza do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="507"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="511"/>
        <source>Copy Email</source>
        <translation>Skopiuj adres e-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="512"/>
        <source>Copy this key&apos;s email address to the clipboard</source>
        <translation>Skopiuj adres e-mail tego klucza do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="516"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="520"/>
        <source>Copy Public Key Block</source>
        <translation>Skopiuj blok klucza publicznego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="521"/>
        <source>Copy this key&apos;s armored public key block to the clipboard, ready to paste or send</source>
        <translation>Skopiuj opakowany blok klucza publicznego tego klucza do schowka, gotowy do wklejenia lub wysłania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="531"/>
        <source>Certify Key...</source>
        <translation>Poświadcz klucz...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="532"/>
        <source>Sign this key&apos;s identity with your key</source>
        <translation>Podpisz tożsamość tego klucza swoim kluczem</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="541"/>
        <source>Set Expiry...</source>
        <translation>Ustaw wygaśnięcie...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="541"/>
        <source>Change when this key expires</source>
        <translation>Zmień moment wygaśnięcia tego klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="553"/>
        <source>Export Public Key...</source>
        <translation>Eksportuj klucz publiczny...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="554"/>
        <source>Save this key&apos;s public half to a file you can send</source>
        <translation>Zapisz publiczną połowę tego klucza do pliku, który możesz wysłać</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="563"/>
        <source>Export Private Key...</source>
        <translation>Eksportuj klucz prywatny...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="564"/>
        <source>Save this key&apos;s private half to a file — keep it to yourself</source>
        <translation>Zapisz prywatną połowę tego klucza do pliku — zachowaj ją dla siebie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="573"/>
        <source>Generate Revocation Certificate...</source>
        <translation>Wygeneruj certyfikat unieważnienia...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="574"/>
        <source>Create a certificate that can revoke this key, to store somewhere safe in case you ever lose control of it</source>
        <translation>Utwórz certyfikat mogący unieważnić ten klucz i przechowuj go w bezpiecznym miejscu na wypadek utraty nad nim kontroli</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="586"/>
        <source>Search Keyserver...</source>
        <translation>Szukaj na serwerze kluczy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="587"/>
        <source>Search a keyserver and import keys</source>
        <translation>Przeszukaj serwer kluczy i zaimportuj klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="603"/>
        <source>Publish Key to Keyserver...</source>
        <translation>Opublikuj klucz na serwerze kluczy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="610"/>
        <source>Refresh Selected from Keyserver</source>
        <translation>Odśwież wybrane z serwera kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="611"/>
        <source>Fetch the latest copy of these key(s) from the key server, picking up new signatures and revocations</source>
        <translation>Pobierz najnowszą kopię tych kluczy z serwera kluczy, wraz z nowymi podpisami i unieważnieniami</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="623"/>
        <source>Set Owner Trust for Checked Keys...</source>
        <translation>Ustaw zaufanie do właściciela dla zaznaczonych kluczy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="624"/>
        <source>Give every checked key the same owner trust level</source>
        <translation>Nadaj każdemu zaznaczonemu kluczowi ten sam poziom zaufania do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="629"/>
        <source>Extend Expiry of Checked Keys...</source>
        <translation>Przedłuż wygaśnięcie zaznaczonych kluczy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="630"/>
        <source>Push back the expiry date of every checked private key in one step</source>
        <translation>Przesuń datę wygaśnięcia każdego zaznaczonego klucza prywatnego w jednym kroku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="636"/>
        <source>Back Up All Private Keys...</source>
        <translation>Kopia zapasowa wszystkich kluczy prywatnych...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="637"/>
        <source>Export every private key in this keyring to a single key package</source>
        <translation>Wyeksportuj wszystkie klucze prywatne z tej bazy kluczy do jednego pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="652"/>
        <source>Edit</source>
        <translation>Edytuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="661"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="717"/>
        <source>Key</source>
        <translation>Klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="662"/>
        <source>Generate Key</source>
        <translation>Wygeneruj klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="666"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="731"/>
        <source>Import Key</source>
        <translation>Importuj klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="671"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="737"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="738"/>
        <source>Export Key</source>
        <translation>Eksportuj klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="687"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="764"/>
        <source>Delete</source>
        <translation>Usuń</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="692"/>
        <source>Operations</source>
        <translation>Operacje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="703"/>
        <source>Bulk</source>
        <translation>Masowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="732"/>
        <source>Import key</source>
        <translation>Importuj klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="752"/>
        <source>Certify, set expiry, add a subkey, set trust, revoke</source>
        <translation>Poświadcz, ustaw wygaśnięcie, dodaj podklucz, ustaw zaufanie, unieważnij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="765"/>
        <source>Delete the selected or the checked keys</source>
        <translation>Usuń wybrane lub zaznaczone klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="771"/>
        <source>View</source>
        <translation>Widok</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="886"/>
        <source>Deleting Keys</source>
        <translation>Usuwanie kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="887"/>
        <source>Are you sure that you want to delete the following keys?</source>
        <translation>Czy na pewno chcesz usunąć następujące klucze?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="889"/>
        <source>The action can not be undone.</source>
        <translation>Tej operacji nie można cofnąć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="940"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="952"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1014"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1242"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1253"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1266"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1298"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1306"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1322"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1330"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1357"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="969"/>
        <source>%1 is empty</source>
        <translation>%1 jest pusty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="973"/>
        <source>%1 copied to clipboard</source>
        <translation>Skopiowano %1 do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1121"/>
        <source>Nothing to Extend</source>
        <translation>Nie ma czego przedłużać</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1122"/>
        <source>None of the %n selected key(s) has a private key, so their expiry cannot be changed. Expiry is set on keys you own.</source>
        <translation>
            <numerusform>Żaden z %n wybranego klucza nie ma klucza prywatnego, więc nie można zmienić jego daty wygaśnięcia. Datę wygaśnięcia ustawia się tylko na kluczach, które posiadasz.</numerusform>
            <numerusform>Żaden z %n wybranych kluczy nie ma klucza prywatnego, więc nie można zmienić ich daty wygaśnięcia. Datę wygaśnięcia ustawia się tylko na kluczach, które posiadasz.</numerusform>
            <numerusform>Żaden z %n wybranych kluczy nie ma klucza prywatnego, więc nie można zmienić ich daty wygaśnięcia. Datę wygaśnięcia ustawia się tylko na kluczach, które posiadasz.</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="987"/>
        <source>Publish Key to Keyserver</source>
        <translation>Opublikuj klucz na serwerze kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="989"/>
        <source>You are about to upload the following public key(s) to the default keyserver:</source>
        <translation>Zamierzasz przesłać następujące klucze publiczne do domyślnego serwera kluczy:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="992"/>
        <source>Publication is &lt;b&gt;permanent and public&lt;/b&gt;: the key(s) cannot be removed from most keyservers once uploaded. Only the public part is uploaded, never your private key.</source>
        <translation>Publikacja jest &lt;b&gt;trwała i publiczna&lt;/b&gt;: po przesłaniu kluczy nie można usunąć ich z większości serwerów kluczy. Przesyłana jest tylko część publiczna, nigdy Twój klucz prywatny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="995"/>
        <source>Do you want to proceed?</source>
        <translation>Czy chcesz kontynuować?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1030"/>
        <source>Upload Failed</source>
        <translation>Przesyłanie nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1040"/>
        <source>The public key was uploaded to the key server.</source>
        <translation>Klucz publiczny został przesłany na serwer kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1042"/>
        <source>The public key was uploaded to %1.</source>
        <translation>Klucz publiczny został przesłany do %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1051"/>
        <source>Fingerprint: %1</source>
        <translation>Odcisk klucza: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1054"/>
        <source>Upload Complete</source>
        <translation>Przesyłanie zakończone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1066"/>
        <source>Undefined</source>
        <translation>Nieokreślone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1066"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1083"/>
        <source>Never</source>
        <translation>Nigdy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1066"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1081"/>
        <source>Marginal</source>
        <translation>Marginalne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1066"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1079"/>
        <source>Full</source>
        <translation>Pełne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1067"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1077"/>
        <source>Ultimate</source>
        <translation>Ostateczne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1071"/>
        <source>Modify Owner Trust Level</source>
        <translation>Zmień poziom zaufania do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1072"/>
        <source>Trust level to apply to %1 checked key(s):</source>
        <translation>Poziom zaufania do zastosowania dla zaznaczonych kluczy (%1):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1099"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1174"/>
        <source>Partially Failed</source>
        <translation>Częściowe niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1100"/>
        <source>Failed to set owner trust on %1 key(s).</source>
        <translation>Nie udało się ustawić poziomu zaufania do właściciela dla %1 kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1129"/>
        <source>Extend Expiry of Checked Keys</source>
        <translation>Przedłuż wygaśnięcie zaznaczonych kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1132"/>
        <source>New expiry date to apply to %1 private key(s):</source>
        <translation>Nowa data wygaśnięcia do zastosowania dla kluczy prywatnych (%1):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1140"/>
        <source>Never expires</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1159"/>
        <source>Extending Expiry</source>
        <translation>Przedłużanie wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1175"/>
        <source>Failed to update expiry on %1 key(s).</source>
        <translation>Nie udało się zaktualizować daty wygaśnięcia %1 kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1193"/>
        <source>No Private Keys</source>
        <translation>Brak kluczy prywatnych</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1194"/>
        <source>There are no private keys in this keyring to back up.</source>
        <translation>W tej bazie kluczy nie ma kluczy prywatnych do wykonania kopii zapasowej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1201"/>
        <source>private key(s) backed up</source>
        <translation>klucze prywatne objęte kopią zapasową</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1336"/>
        <source>Enter PIN</source>
        <translation>Wprowadź PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1337"/>
        <source>Please enter PIN to decrypt the Key:</source>
        <translation>Wprowadź PIN, aby odszyfrować klucz:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1602"/>
        <source>Category (%1 keys)</source>
        <translation>Kategoria (kluczy: %1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1614"/>
        <source>Remove From This Category</source>
        <translation>Usuń z tej kategorii</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1650"/>
        <source>New Category...</source>
        <translation>Nowa kategoria...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1654"/>
        <source>New Category</source>
        <translation>Nowa kategoria</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1654"/>
        <source>Category name:</source>
        <translation>Nazwa kategorii:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1689"/>
        <source>%1 keys · %2 private · %3 expiring soon · %4 expired or revoked</source>
        <translation>Klucze: %1 · prywatne: %2 · wygasające wkrótce: %3 · wygasłe lub unieważnione: %4</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1696"/>
        <source>%1 key(s) expire within %2 days. See the &quot;Expiring Soon&quot; tab.</source>
        <translation>%1 kluczy wygasa w ciągu %2 dni. Zobacz kartę „Wygasa wkrótce”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1700"/>
        <source>No key expires within the next %1 days.</source>
        <translation>Żaden klucz nie wygasa w ciągu najbliższych %1 dni.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="604"/>
        <source>Upload the checked public key(s) to the key server configured as the default</source>
        <translation>Prześlij zaznaczone klucze publiczne na serwer kluczy ustawiony jako domyślny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="916"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1281"/>
        <source>key(s) exported</source>
        <translation>klucze wyeksportowano</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="930"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1000"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1232"/>
        <source>Exporting</source>
        <translation>Eksportowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="941"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="953"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1015"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1243"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1254"/>
        <source>Unknown error occurred</source>
        <translation>Wystąpił nieznany błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1213"/>
        <source>Invalid Operation</source>
        <translation>Nieprawidłowa operacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1214"/>
        <source>If a key pair does not have a private key then it will not be able to generate sub-keys.</source>
        <translation>Jeśli para kluczy nie ma klucza prywatnego, nie będzie mogła generować podkluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1267"/>
        <source>This key may not be able to export as OpenSSH format. Please check the key-size of the subkey(s) used to sign.</source>
        <translation>Ten klucz może nie dać się wyeksportować w formacie OpenSSH. Sprawdź rozmiar podkluczy używanych do podpisywania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1276"/>
        <source>Export OpenSSH Key To File</source>
        <translation>Eksportuj klucz OpenSSH do pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1277"/>
        <source>OpenSSH Public Key Files</source>
        <translation>Pliki kluczy publicznych OpenSSH</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1289"/>
        <source>Import Key Package</source>
        <translation>Importuj pakiet kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1299"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1323"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Nie można otworzyć tego pliku. Upewnij się, że jest to zwykły plik i jest możliwy do odczytania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1307"/>
        <source>The target file is too large for a key package.</source>
        <translation>Plik docelowy jest zbyt duży dla pakietu kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1312"/>
        <source>Import Key Package Passphrase File</source>
        <translation>Importuj plik hasła pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1313"/>
        <source>Key Package Passphrase File</source>
        <translation>Plik hasła pakietu kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1331"/>
        <source>The target file is too large for a key package passphrase.</source>
        <translation>Plik docelowy jest zbyt duży dla hasła pakietu kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1363"/>
        <source>key(s) imported</source>
        <translation>klucze zaimportowano</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyNewUIDDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="45"/>
        <source>Add a new User ID</source>
        <translation>Dodaj nowy identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="52"/>
        <source>A User ID pairs a name with an optional email and comment. The name is required.</source>
        <translation>Identyfikator użytkownika łączy nazwę z opcjonalnym e-mailem i komentarzem. Nazwa jest wymagana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="58"/>
        <source>Full name</source>
        <translation>Imię i nazwisko</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="61"/>
        <source>name@example.com</source>
        <translation>nazwa@przyklad.com</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="64"/>
        <source>Optional comment</source>
        <translation>Opcjonalny komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="70"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="71"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="72"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="79"/>
        <source>Create</source>
        <translation>Utwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="90"/>
        <source>The new User ID will be set as the primary User ID.</source>
        <translation>Nowy identyfikator użytkownika zostanie ustawiony jako główny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="139"/>
        <source>Enter a name for the User ID.</source>
        <translation>Wprowadź nazwę dla identyfikatora użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="152"/>
        <source>Please give a valid email address.</source>
        <translation>Podaj prawidłowy adres e-mail.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="154"/>
        <source>Ready to create the User ID.</source>
        <translation>Można utworzyć identyfikator użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="196"/>
        <source>Successful Operation</source>
        <translation>Operacja zakończona pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="197"/>
        <source>Successfully added a new UID.</source>
        <translation>Pomyślnie dodano nowy identyfikator użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="205"/>
        <source>Operation Failed</source>
        <translation>Operacja nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="206"/>
        <source>An error occurred during the operation.</source>
        <translation>Wystąpił błąd podczas operacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="116"/>
        <source>Create New UID</source>
        <translation>Utwórz nowy identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="146"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Nazwa i komentarz nie mogą zawierać znaków &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; ani znaków kontrolnych.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="210"/>
        <source>Creating UID</source>
        <translation>Tworzenie identyfikatora użytkownika</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairDetailTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="63"/>
        <source>Owner</source>
        <translation>Właściciel</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="64"/>
        <source>Primary Key</source>
        <translation>Klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="65"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="66"/>
        <source>Additional UIDs</source>
        <translation>Dodatkowe identyfikatory użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="100"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="101"/>
        <source>Email Address</source>
        <translation>Adres e-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="102"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="115"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="130"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="134"/>
        <source>Algorithm Detail</source>
        <translation>Szczegóły algorytmu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="139"/>
        <source>Key Format Version</source>
        <translation>Wersja formatu klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="147"/>
        <source>Key Size</source>
        <translation>Rozmiar klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="151"/>
        <source>Usage</source>
        <translation>Zastosowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="156"/>
        <source>Owner Trust Level</source>
        <translation>Poziom zaufania do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="164"/>
        <source>Create Date (Local Time)</source>
        <translation>Data utworzenia (czas lokalny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="170"/>
        <source>Expires on (Local Time)</source>
        <translation>Wygasa (czas lokalny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="178"/>
        <source>Last Update (Local Time)</source>
        <translation>Ostatnia aktualizacja (czas lokalny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="183"/>
        <source>Primary Key Existence</source>
        <translation>Istnienie klucza głównego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="417"/>
        <source>Notice: The public key has been published on the key server.</source>
        <translation>Uwaga: klucz publiczny został opublikowany na serwerze kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="419"/>
        <source>Notice: The public key has been published on %1.</source>
        <translation>Uwaga: klucz publiczny został opublikowany na %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="118"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="215"/>
        <source>Copy</source>
        <translation>Skopiuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="217"/>
        <source>copy fingerprint to clipboard</source>
        <translation>skopiuj odcisk klucza do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="267"/>
        <source>Exists</source>
        <translation>Istnieje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="267"/>
        <source>Not Exists</source>
        <translation>Nie istnieje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="290"/>
        <source>Certificate</source>
        <translation>Certyfikacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="292"/>
        <source>Encrypt</source>
        <translation>Szyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="293"/>
        <source>Sign</source>
        <translation>Podpisywanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="294"/>
        <source>Auth</source>
        <translation>Uwierzytelnianie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="320"/>
        <source>Never Expire</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="339"/>
        <source>No Data</source>
        <translation>Brak danych</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="353"/>
        <source>Warning: The primary key has expired.</source>
        <translation>Ostrzeżenie: klucz główny wygasł.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="356"/>
        <source>Warning: The primary key has been revoked.</source>
        <translation>Ostrzeżenie: klucz główny został unieważniony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="359"/>
        <source>Warning: The primary key is not exists.</source>
        <translation>Ostrzeżenie: klucz główny nie istnieje.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairOperaTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="65"/>
        <source>General Operations</source>
        <translation>Operacje ogólne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="71"/>
        <source>Export Public Key</source>
        <translation>Eksportuj klucz publiczny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="77"/>
        <source>Export Private Key</source>
        <translation>Eksportuj klucz prywatny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="87"/>
        <source>Modify Expiration Datetime (Primary Key)</source>
        <translation>Zmień datę i czas wygaśnięcia (klucz główny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="94"/>
        <source>Modify Password</source>
        <translation>Zmień hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="103"/>
        <source>Revoke Certificate Operation</source>
        <translation>Operacja certyfikatu unieważnienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="112"/>
        <source>Set Owner Trust Level</source>
        <translation>Ustaw poziom zaufania do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="150"/>
        <source>Export Full Secret Key</source>
        <translation>Eksportuj pełny klucz prywatny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="158"/>
        <source>Export Shortest Secret Key</source>
        <translation>Eksportuj najkrótszy klucz prywatny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="166"/>
        <source>Generate Revoke Certificate</source>
        <translation>Wygeneruj certyfikat unieważnienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="171"/>
        <source>Import Revoke Certificate</source>
        <translation>Importuj certyfikat unieważnienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="236"/>
        <source>Revocation Certificates</source>
        <translation>Certyfikaty unieważnienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="223"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="235"/>
        <source>Import Key Revocation Certificate</source>
        <translation>Importuj certyfikat unieważnienia klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="224"/>
        <source>You are about to import the</source>
        <translation>Zamierzasz zaimportować</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="225"/>
        <source>REVOCATION CERTIFICATE</source>
        <translation>CERTYFIKAT UNIEWAŻNIENIA</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="226"/>
        <source>A successful import will result in the key being irreversibly revoked.</source>
        <translation>Pomyślny import spowoduje nieodwracalne unieważnienie klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="228"/>
        <source>Do you REALLY want to execute this operation?</source>
        <translation>Czy NAPRAWDĘ chcesz wykonać tę operację?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="244"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="253"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="261"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="245"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="262"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Nie można otworzyć tego pliku. Upewnij się, że jest to zwykły plik i jest możliwy do odczytania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="254"/>
        <source>The target file is too large for a key revocation certificate.</source>
        <translation>Plik docelowy jest zbyt duży dla certyfikatu unieważnienia klucza.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairPhotosTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Flags</source>
        <translation>Flagi</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Date</source>
        <translation>Data</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Size</source>
        <translation>Rozmiar</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Thumbnail</source>
        <translation>Miniatura</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="99"/>
        <source>List of Photo IDs (%1)</source>
        <translation>Lista identyfikatorów zdjęć (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="100"/>
        <source>Photo Viewer</source>
        <translation>Przeglądarka zdjęć</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairSubkeyTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="148"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="266"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="158"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="266"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="163"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="267"/>
        <source>Algorithm Detail</source>
        <translation>Szczegóły algorytmu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="169"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="266"/>
        <source>Key Size</source>
        <translation>Rozmiar klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="174"/>
        <source>Usage</source>
        <translation>Zastosowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="179"/>
        <source>Expires On (Local Time)</source>
        <translation>Wygasa (czas lokalny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="186"/>
        <source>Create Date (Local Time)</source>
        <translation>Data utworzenia (czas lokalny)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="190"/>
        <source>Existence</source>
        <translation>Istnienie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="201"/>
        <source>Key in Smart Card</source>
        <translation>Klucz na karcie inteligentnej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="206"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="78"/>
        <source>List of the primary key and subkey(s)</source>
        <translation>Lista klucza głównego i podkluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="79"/>
        <source>Detail of Selected Primary Key/Subkey</source>
        <translation>Szczegóły wybranego klucza głównego/podklucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="83"/>
        <source>New Subkey</source>
        <translation>Nowy podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="84"/>
        <source>Add ADSK(s)</source>
        <translation>Dodaj ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="153"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="266"/>
        <source>Key Type</source>
        <translation>Typ klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="196"/>
        <source>Revoked</source>
        <translation>Unieważniony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="101"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="463"/>
        <source>Export Subkey</source>
        <translation>Eksportuj podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="267"/>
        <source>Create Date</source>
        <translation>Data utworzenia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="270"/>
        <source>Expire Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="293"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="468"/>
        <source>Primary Key</source>
        <translation>Klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="293"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="469"/>
        <source>Subkey</source>
        <translation>Podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="294"/>
        <source>ADSK</source>
        <translation>ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="320"/>
        <source>Never Expire</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="442"/>
        <source>Never Expires</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="404"/>
        <source>Exists</source>
        <translation>Istnieje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="106"/>
        <source>Exporting subkeys is not supported by the current OpenPGP backend.</source>
        <translation>Eksportowanie podkluczy nie jest obsługiwane przez bieżący silnik OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="405"/>
        <source>Not Exists</source>
        <translation>Nie istnieje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="408"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="471"/>
        <source>Yes</source>
        <translation>Tak</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="408"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="471"/>
        <source>No</source>
        <translation>Nie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="462"/>
        <source>Export Primary Key</source>
        <translation>Eksportuj klucz główny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="481"/>
        <source>Edit Expire Date</source>
        <translation>Zmień datę wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="485"/>
        <source>Export</source>
        <translation>Eksportuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="489"/>
        <source>Delete</source>
        <translation>Usuń</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="493"/>
        <source>Revoke</source>
        <translation>Unieważnij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="497"/>
        <source>Change Passphrase</source>
        <translation>Zmień hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="501"/>
        <source>Move to Card</source>
        <translation>Przenieś na kartę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="623"/>
        <source>Exporting Subkey</source>
        <translation>Eksportowanie podklucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="624"/>
        <source>You are about to export a private subkey.</source>
        <translation>Zamierzasz wyeksportować prywatny podklucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="625"/>
        <source>While subkeys are less critical than the primary key, they should still be handled with care.</source>
        <translation>Choć podklucze są mniej krytyczne niż klucz główny, nadal należy się z nimi obchodzić ostrożnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="628"/>
        <source>Do you want to proceed with exporting this subkey?</source>
        <translation>Czy chcesz kontynuować eksport tego podklucza?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="655"/>
        <source>Export Key To File</source>
        <translation>Eksportuj klucz do pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="656"/>
        <source>Key Files</source>
        <translation>Pliki kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="661"/>
        <source>Export Error</source>
        <translation>Błąd eksportu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="662"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Nie można otworzyć %1 do zapisu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="672"/>
        <source>&lt;h3&gt;You are about to delete the subkey:&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;This action is irreversible. Please confirm.</source>
        <translation>&lt;h3&gt;Zamierzasz usunąć podklucz:&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;Identyfikator klucza:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Tej operacji nie można cofnąć. Potwierdź.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="678"/>
        <source>Delete Subkey Confirmation</source>
        <translation>Potwierdzenie usunięcia podklucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="693"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="778"/>
        <source>Illegal Operation</source>
        <translation>Nieprawidłowa operacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="694"/>
        <source>Cannot delete the primary key or an invalid subkey.</source>
        <translation>Nie można usunąć klucza głównego ani nieprawidłowego podklucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="702"/>
        <source>Operation Failed</source>
        <translation>Operacja nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="703"/>
        <source>The selected subkey could not be deleted. Please check your permissions or try again.</source>
        <translation>Nie udało się usunąć wybranego podklucza. Sprawdź uprawnienia lub spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="709"/>
        <source>Operation Successful</source>
        <translation>Operacja zakończona pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="710"/>
        <source>The subkey with KeyID %1 has been successfully deleted.</source>
        <translation>Podklucz o identyfikatorze %1 został pomyślnie usunięty.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="755"/>
        <source>&lt;h3&gt;Revoke Subkey Confirmation&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Revoking a subkey will make it permanently unusable. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br /&gt;Are you sure you want to revoke this subkey?</source>
        <translation>&lt;h3&gt;Potwierdzenie unieważnienia podklucza&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;Identyfikator klucza:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Unieważnienie podklucza uczyni go trwale niezdatnym do użycia. Ta operacja jest &lt;b&gt;nieodwracalna&lt;/b&gt;.&lt;br /&gt;Czy na pewno chcesz unieważnić ten podklucz?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="762"/>
        <source>Revoke Subkey</source>
        <translation>Unieważnij podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="779"/>
        <source>Cannot revoke the primary key or an invalid subkey.</source>
        <translation>Nie można unieważnić klucza głównego ani nieprawidłowego podklucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="784"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Bez powodu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="784"/>
        <source>1 -&gt; This key is no more safe.</source>
        <translation>1 -&gt; Ten klucz nie jest już bezpieczny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="785"/>
        <source>2 -&gt; Key is outdated.</source>
        <translation>2 -&gt; Klucz jest przestarzały.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="785"/>
        <source>3 -&gt; Key is no longer used</source>
        <translation>3 -&gt; Klucz nie jest już używany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="797"/>
        <source>Revocation Failed</source>
        <translation>Unieważnienie nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="798"/>
        <source>Failed to revoke the subkey. Please try again.</source>
        <translation>Nie udało się unieważnić podklucza. Spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="801"/>
        <source>Revocation Successful</source>
        <translation>Unieważnienie zakończone pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="802"/>
        <source>The subkey has been successfully revoked.</source>
        <translation>Podklucz został pomyślnie unieważniony.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairUIDTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="57"/>
        <source>New UID</source>
        <translation>Nowy identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="72"/>
        <source>UIDs</source>
        <translation>Identyfikatory użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="83"/>
        <source>Signature of Selected UID</source>
        <translation>Podpis wybranego identyfikatora użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Email</source>
        <translation>E-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <source>Comment</source>
        <translation>Komentarz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <source>Index</source>
        <translation>Indeks</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Create Date</source>
        <translation>Data utworzenia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="155"/>
        <source>Expired Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="261"/>
        <source>Never Expires</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="298"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="453"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="462"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="508"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="529"/>
        <source>Invalid Operation</source>
        <translation>Nieprawidłowa operacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="307"/>
        <source>Are you sure that you want to delete the following UID?</source>
        <translation>Czy na pewno chcesz usunąć następujący identyfikator użytkownika?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="328"/>
        <source>Successfully deleted the UID.</source>
        <translation>Pomyślnie usunięto identyfikator użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="332"/>
        <source>Deleting UID</source>
        <translation>Usuwanie identyfikatora użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="372"/>
        <source>Setting Primary UID</source>
        <translation>Ustawianie głównego identyfikatora użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="515"/>
        <source>&lt;h3&gt;Revoke UID Confirmation&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;UID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Revoking a UID will make it permanently unusable. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br /&gt;Are you sure you want to revoke this UID?</source>
        <translation>&lt;h3&gt;Potwierdzenie unieważnienia identyfikatora użytkownika&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;Identyfikator:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Unieważnienie identyfikatora użytkownika uczyni go trwale niezdatnym do użycia. Ta operacja jest &lt;b&gt;nieodwracalna&lt;/b&gt;.&lt;br /&gt;Czy na pewno chcesz unieważnić ten identyfikator użytkownika?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="535"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Bez powodu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="535"/>
        <source>4 -&gt; User ID is no longer valid.</source>
        <translation>4 -&gt; Identyfikator użytkownika nie jest już ważny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="550"/>
        <source>Revocation Failed</source>
        <translation>Unieważnienie nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="551"/>
        <source>Failed to revoke the UID. Please try again.</source>
        <translation>Nie udało się unieważnić identyfikatora użytkownika. Spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="555"/>
        <source>Revocation Successful</source>
        <translation>Unieważnienie zakończone pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="556"/>
        <source>The UID has been successfully revoked.</source>
        <translation>Identyfikator użytkownika został pomyślnie unieważniony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="560"/>
        <source>Revoking UID</source>
        <translation>Unieważnianie identyfikatora użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="327"/>
        <source>Successful Operation</source>
        <translation>Operacja zakończona pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="321"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="365"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="484"/>
        <source>Operation Failed</source>
        <translation>Operacja nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="366"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="485"/>
        <source>An error occurred during the operation.</source>
        <translation>Wystąpił błąd podczas operacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="299"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="530"/>
        <source>Cannot delete the Primary UID.</source>
        <translation>Nie można usunąć głównego identyfikatora użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="304"/>
        <source>Deleting UIDs</source>
        <translation>Usuwanie identyfikatorów użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="309"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="352"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="478"/>
        <source>The action can not be undone.</source>
        <translation>Tej operacji nie można cofnąć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="322"/>
        <source>An error occurred during the delete %1 operation.</source>
        <translation>Wystąpił błąd podczas operacji usuwania %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="349"/>
        <source>Set Primary UID</source>
        <translation>Ustaw główny identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="350"/>
        <source>Are you sure that you want to set the Primary UID to?</source>
        <translation>Czy na pewno chcesz ustawić ten identyfikator jako główny?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="389"/>
        <source>Set As Primary</source>
        <translation>Ustaw jako główny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="392"/>
        <source>Sign UID</source>
        <translation>Podpisz identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="395"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="522"/>
        <source>Revoke UID</source>
        <translation>Unieważnij identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="398"/>
        <source>Delete UID</source>
        <translation>Usuń identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="509"/>
        <source>Please select one UID before doing this operation.</source>
        <translation>Przed tą operacją wybierz jeden identyfikator użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="442"/>
        <source>Delete(Revoke) Key Signature</source>
        <translation>Usuń(unieważnij) podpis klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="454"/>
        <source>Please select one Key Signature before doing this operation.</source>
        <translation>Przed tą operacją wybierz jeden podpis klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="463"/>
        <source>To delete the signature, you need to have its corresponding public key in the local database.</source>
        <translation>Aby usunąć podpis, musisz mieć odpowiadający mu klucz publiczny w lokalnej bazie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="473"/>
        <source>Deleting Key Signature</source>
        <translation>Usuwanie podpisu klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="475"/>
        <source>Are you sure that you want to delete the following signature?</source>
        <translation>Czy na pewno chcesz usunąć następujący podpis?</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeySetExpireDateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="110"/>
        <source>1 year</source>
        <translation>1 rok</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="110"/>
        <source>%1 years</source>
        <translation>%1 lat</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="113"/>
        <source>1 month</source>
        <translation>1 miesiąc</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="113"/>
        <source>%1 months</source>
        <translation>%1 miesięcy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="117"/>
        <source>1 day</source>
        <translation>1 dzień</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="117"/>
        <source>%1 days</source>
        <translation>%1 dni</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="120"/>
        <source>, </source>
        <translation>, </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="137"/>
        <source>Subkey Expiration Date</source>
        <translation>Data wygaśnięcia podklucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="138"/>
        <source>Key Expiration Date</source>
        <translation>Data wygaśnięcia klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="145"/>
        <source>The subkey can no longer sign or encrypt after this moment. You can extend it again later.</source>
        <translation>Po tym momencie podklucz nie będzie już mógł podpisywać ani szyfrować. Możesz go później przedłużyć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="147"/>
        <source>The key can no longer sign or encrypt after this moment. You can extend it again later.</source>
        <translation>Po tym momencie klucz nie będzie już mógł podpisywać ani szyfrować. Możesz go później przedłużyć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="186"/>
        <source>Validity Period</source>
        <translation>Okres ważności</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="187"/>
        <source>Expires On</source>
        <translation>Wygasa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="224"/>
        <source>Currently set to never expire.</source>
        <translation>Obecnie nigdy nie wygasa.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="229"/>
        <source>Currently expired since %1.</source>
        <translation>Obecnie wygasł %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="231"/>
        <source>Currently expires %1 · %2 left</source>
        <translation>Obecnie wygasa %1 · pozostało %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="245"/>
        <source>Modify Subkey Expiration Date</source>
        <translation>Zmień datę wygaśnięcia podklucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="246"/>
        <source>Modify Key Expiration Date</source>
        <translation>Zmień datę wygaśnięcia klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="269"/>
        <source>Will never expire.</source>
        <translation>Nigdy nie wygaśnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="271"/>
        <source>The expiration date must be in the future.</source>
        <translation>Data wygaśnięcia musi być w przyszłości.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="273"/>
        <source>Valid for %1 — until %2</source>
        <translation>Ważność: %1 — do %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="320"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="321"/>
        <source>The expire date of the key pair has been updated.</source>
        <translation>Data wygaśnięcia pary kluczy została zaktualizowana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="330"/>
        <source>Failure</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="331"/>
        <source>Failed to update the expire date of the key pair.</source>
        <translation>Nie udało się zaktualizować daty wygaśnięcia pary kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="120"/>
        <source>Custom Date</source>
        <translation>Własna data</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="121"/>
        <source>3 Months</source>
        <translation>3 miesiące</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="123"/>
        <source>6 Months</source>
        <translation>6 miesięcy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="125"/>
        <source>1 Year</source>
        <translation>1 rok</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="127"/>
        <source>2 Years</source>
        <translation>2 lata</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="129"/>
        <source>5 Years</source>
        <translation>5 lat</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="131"/>
        <source>10 Years</source>
        <translation>10 lat</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="133"/>
        <source>Never Expires</source>
        <translation>Nigdy nie wygasa</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyTableEmptyState</name>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="57"/>
        <source>No keys yet.

Use Key ▸ Generate Key to make one, or Key ▸ Import Key to bring in one you already have.</source>
        <translation>Nie ma jeszcze kluczy.

Użyj Klucz ▸ Wygeneruj klucz, aby go utworzyć, albo Klucz ▸ Importuj klucz, aby dodać klucz, który już masz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="63"/>
        <source>No key matches &quot;%1&quot;.

Clear the search to see every key again.</source>
        <translation>Żaden klucz nie pasuje do „%1”.

Wyczyść wyszukiwanie, aby znów zobaczyć wszystkie klucze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="70"/>
        <source>This category has no keys yet.

Right-click a key in another tab and use Category to file it here.</source>
        <translation>Ta kategoria nie ma jeszcze kluczy.

Kliknij klucz w innej karcie prawym przyciskiem i użyj Kategoria, aby go tu umieścić.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="76"/>
        <source>No key in this keyring belongs in this tab.</source>
        <translation>Żaden klucz z tej bazy kluczy nie należy do tej karty.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyUIDSignDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="52"/>
        <source>Signers</source>
        <translation>Podpisujący</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="93"/>
        <source>Expire Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="103"/>
        <source>Sign For Key&apos;s UID(s)</source>
        <translation>Podpisz identyfikatory użytkownika klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="125"/>
        <source>Unsuccessful Operation</source>
        <translation>Operacja nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="126"/>
        <source>Signature operation failed for UID %1</source>
        <translation>Operacja podpisania nie powiodła się dla identyfikatora użytkownika %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="130"/>
        <source>Operation Complete</source>
        <translation>Operacja zakończona</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="131"/>
        <source>The signature operation of the UID is complete</source>
        <translation>Operacja podpisania identyfikatora użytkownika została zakończona</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::LogViewDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="92"/>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="96"/>
        <source>Application Logs</source>
        <translation>Dzienniki aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="102"/>
        <source>No entries</source>
        <translation>Brak wpisów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="112"/>
        <source>Filter logs…</source>
        <translation>Filtruj dzienniki…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="130"/>
        <source>Refresh</source>
        <translation>Odśwież</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="131"/>
        <source>Copy</source>
        <translation>Skopiuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="132"/>
        <source>Save</source>
        <translation>Zapisz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="133"/>
        <source>Clear View</source>
        <translation>Wyczyść widok</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="134"/>
        <source>Close</source>
        <translation>Zamknij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="137"/>
        <source>Auto Refresh</source>
        <translation>Automatyczne odświeżanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="249"/>
        <source>%1 entries</source>
        <translation>Wpisów: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="260"/>
        <source>Save Logs</source>
        <translation>Zapisz dzienniki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="263"/>
        <source>Text Files (*.txt);;All Files (*)</source>
        <translation>Pliki tekstowe (*.txt);;Wszystkie pliki (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="269"/>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="284"/>
        <source>Save Failed</source>
        <translation>Zapis nie powiódł się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="270"/>
        <source>Unable to open file for writing.</source>
        <translation>Nie można otworzyć pliku do zapisu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="285"/>
        <source>Failed to save the log file.</source>
        <translation>Nie udało się zapisać pliku dziennika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="289"/>
        <source>Saved</source>
        <translation>Zapisano</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="290"/>
        <source>Logs have been saved successfully.</source>
        <translation>Dzienniki zostały pomyślnie zapisane.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::MainWindow</name>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="168"/>
        <source>GUI Pinentry Not Found</source>
        <translation>Nie znaleziono graficznego Pinentry</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="169"/>
        <source>No suitable *graphical* Pinentry program was found on your system.

Please install a GUI-based Pinentry (e.g., &apos;pinentry-qt&apos;, &apos;pinentry-gnome3&apos;, or &apos;pinentry-mac&apos; on macOS).

Without a GUI Pinentry, GnuPG cannot prompt you for passwords or passphrases.

After installing it, please restart GpgFrontend. The configuration file will be updated automatically.</source>
        <translation>Nie znaleziono w Twoim systemie odpowiedniego *graficznego* programu Pinentry.

Zainstaluj graficzny Pinentry (np. „pinentry-qt”, „pinentry-gnome3” albo „pinentry-mac” w macOS).

Bez graficznego Pinentry GnuPG nie może pytać Cię o hasła.

Po instalacji uruchom GpgFrontend ponownie. Plik konfiguracyjny zostanie zaktualizowany automatycznie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="195"/>
        <source>Add To Category</source>
        <translation>Dodaj do kategorii</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="255"/>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="258"/>
        <source>Critical error occur while loading GpgFrontend.</source>
        <translation>Wystąpił błąd krytyczny podczas ładowania GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="257"/>
        <source>Loading Failed</source>
        <translation>Ładowanie nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="395"/>
        <source>Remove From This Category</source>
        <translation>Usuń z tej kategorii</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="425"/>
        <source>New Category...</source>
        <translation>Nowa kategoria...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="429"/>
        <source>New Category</source>
        <translation>Nowa kategoria</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="429"/>
        <source>Category name:</source>
        <translation>Nazwa kategorii:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="461"/>
        <source>Name: %1</source>
        <translation>Nazwa: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="462"/>
        <source>Path: %1</source>
        <translation>Ścieżka: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="475"/>
        <source>Invalid Key Databases</source>
        <translation>Nieprawidłowe bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="480"/>
        <source>Detected %1 invalid key database(s).</source>
        <translation>Wykryto nieprawidłowych baz kluczy: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="483"/>
        <source>The application cannot load these databases. Please review the details below and fix the issues in the GnuPG Controller.</source>
        <translation>Aplikacja nie może wczytać tych baz. Przejrzyj szczegóły poniżej i napraw problemy w kontrolerze GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="170"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="285"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="324"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="738"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="303"/>
        <source>Warning</source>
        <translation>Ostrzeżenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="350"/>
        <source>Invalid KeyPair</source>
        <translation>Nieprawidłowa para kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="242"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="539"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="676"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1048"/>
        <source>The selected keypair cannot be used for encryption.</source>
        <translation>Wybrana para kluczy nie może być użyta do szyfrowania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="115"/>
        <source>Instant Messaging</source>
        <translation>Komunikatory</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="122"/>
        <source>Encoding</source>
        <translation>Kodowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="124"/>
        <source>Container Format</source>
        <translation>Format kontenera</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="126"/>
        <source>Message Book</source>
        <translation>Książka Wiadomości</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="128"/>
        <source>Shared phrase (Argon2id)</source>
        <translation>Wspólna fraza (Argon2id)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="129"/>
        <source>Default — no shared phrase set</source>
        <translation>Domyślna — brak wspólnej frazy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="134"/>
        <source>Book Fingerprint</source>
        <translation>Odcisk książki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="139"/>
        <source>Set a Phrase</source>
        <translation>Ustaw frazę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="140"/>
        <source>Settings → Instant Messaging</source>
        <translation>Ustawienia → Komunikatory</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="144"/>
        <source>OpenPGP Payload</source>
        <translation>Dane OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="145"/>
        <source>%1 bytes</source>
        <translation>%1 bajtów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="148"/>
        <source>Token Length</source>
        <translation>Długość tokena</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="149"/>
        <source>%1 characters</source>
        <translation>%1 znaków</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="157"/>
        <source>Wire Overhead</source>
        <translation>Narzut transmisji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="171"/>
        <source>Failed to finalize output file:

%1</source>
        <translation>Nie udało się sfinalizować pliku wyjściowego:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="215"/>
        <source>The operation succeeded, but GpgFrontend failed to finalize one or more output files.

Temporary output files have been cleaned up. Original files were kept unchanged.</source>
        <translation>Operacja powiodła się, ale GpgFrontend nie zdołał sfinalizować jednego lub więcej plików wyjściowych.

Pliki tymczasowe zostały usunięte. Oryginalne pliki pozostały niezmienione.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="286"/>
        <source>Cannot read from the following files:

%1</source>
        <translation>Nie można odczytać następujących plików:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="299"/>
        <source>The target file &quot;%1&quot; already exists.

It will only be replaced after the operation succeeds.
Do you want to continue?</source>
        <translation>Plik docelowy „%1” już istnieje.

Zostanie zastąpiony dopiero po pomyślnym zakończeniu operacji.
Czy chcesz kontynuować?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="325"/>
        <source>Cannot write to the following files:

%1</source>
        <translation>Nie można zapisywać do następujących plików:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="352"/>
        <source>For example the Following Key:</source>
        <translation>Na przykład następujący klucz:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="430"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="702"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="901"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="643"/>
        <source>Encrypting</source>
        <translation>Szyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="498"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="507"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="949"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="533"/>
        <source>Decrypting</source>
        <translation>Odszyfrowywanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="339"/>
        <source>No Key Checked</source>
        <translation>Nie zaznaczono klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="340"/>
        <source>Please check the key in the key toolbox on the right.</source>
        <translation>Zaznacz klucz w przyborniku kluczy po prawej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="608"/>
        <source>No Message Book Phrase Set</source>
        <translation>Brak frazy Książki Wiadomości</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="609"/>
        <source>You have not set a Message Book phrase.</source>
        <translation>Fraza Książki Wiadomości nie została ustawiona.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="610"/>
        <source>Instant messages are hidden using a shared &quot;Message Book&quot;. Without a phrase, GpgFrontend falls back to the built-in default book and that book ships in every copy of the program. It hides the format from a simple scanner, but anyone who knows GpgFrontend can still recognise your message for what it is.

Your message is OpenPGP-encrypted either way; what is at stake here is only whether it is recognisable as an encrypted message at all.

To get that, set a phrase and share it privately with the person you are writing to. You must both use exactly the same one.</source>
        <translation>Wiadomości na komunikatorach są ukrywane za pomocą wspólnej „Książki Wiadomości”. Bez frazy GpgFrontend sięga po wbudowaną domyślną książkę, a ta książka jest dostarczana w każdej kopii programu. Ukrywa ona format przed prostym skanerem, ale każdy, kto zna GpgFrontend, nadal rozpozna Twoją wiadomość za to, czym jest.

Twoja wiadomość jest tak czy inaczej zaszyfrowana OpenPGP; chodzi tu wyłącznie o to, czy w ogóle da się rozpoznać, że to zaszyfrowana wiadomość.

Aby to uzyskać, ustaw frazę i przekaż ją prywatnie osobie, do której piszesz. Oboje musicie używać dokładnie tej samej frazy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="622"/>
        <source>Open Settings…</source>
        <translation>Otwórz ustawienia…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="624"/>
        <source>Continue with Default</source>
        <translation>Kontynuuj z domyślną</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="626"/>
        <source>Continue, Don&apos;t Ask Again</source>
        <translation>Kontynuuj, nie pytaj ponownie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="725"/>
        <source>Message Too Long</source>
        <translation>Wiadomość za długa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="726"/>
        <source>This message is too long to send as an instant message.

The encrypted message is %1 bytes, and the instant-messaging format carries at most %2. Shorten the text, or send it as a normal OpenPGP message instead.</source>
        <translation>Ta wiadomość jest za długa, aby wysłać ją jako wiadomość komunikatora.

Zaszyfrowana wiadomość ma %1 bajtów, a format komunikatora przenosi maksymalnie %2. Skróć tekst albo wyślij ją zamiast tego jako zwykłą wiadomość OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="739"/>
        <source>Failed to prepare the instant message: the encrypted message could not be converted into a token.</source>
        <translation>Nie udało się przygotować wiadomości komunikatora: zaszyfrowanej wiadomości nie udało się przekształcić w token.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="749"/>
        <source>Encrypt Sign</source>
        <translation>Szyfrowanie i podpisywanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="754"/>
        <source>Message encrypted and signed for instant messaging.</source>
        <translation>Wiadomość zaszyfrowana i podpisana do komunikatora.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="755"/>
        <source>Message encrypted for instant messaging.</source>
        <translation>Wiadomość zaszyfrowana do komunikatora.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="757"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="851"/>
        <source>An Instant Messaging section followed by the OpenPGP result.</source>
        <translation>Sekcja komunikatora, a po niej wynik OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="777"/>
        <source>Checking Message</source>
        <translation>Sprawdzanie wiadomości</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="794"/>
        <source>Preparing Instant Message</source>
        <translation>Przygotowywanie wiadomości komunikatora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="842"/>
        <source>Failed to decrypt instant message.</source>
        <translation>Nie udało się odszyfrować wiadomości komunikatora.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="844"/>
        <source>Instant message decrypted (not signed).</source>
        <translation>Wiadomość komunikatora odszyfrowana (niepodpisana).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="846"/>
        <source>Instant message decrypted.</source>
        <translation>Wiadomość komunikatora odszyfrowana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="960"/>
        <source>The selected key contains a key that does not actually have a sign usage.</source>
        <translation>Wybrany klucz zawiera klucz, który w rzeczywistości nie ma zastosowania do podpisywania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="480"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="988"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="715"/>
        <source>Signing</source>
        <translation>Podpisywanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1014"/>
        <source>File to be Verified</source>
        <translation>Plik do zweryfikowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1015"/>
        <source>Please provide An ABSOLUTE Path 
If Data And Signature is COMBINED within a single file, KEEP THIS EMPTY: </source>
        <translation>Podaj ścieżkę BEZWZGLĘDNĄ 
Jeśli dane i podpis są POŁĄCZONE w jednym pliku, ZOSTAW TO PUSTE: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="521"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1034"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="585"/>
        <source>Verifying</source>
        <translation>Weryfikowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="561"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="702"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1096"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="806"/>
        <source>Encrypting and Signing</source>
        <translation>Szyfrowanie i podpisywanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="576"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="589"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1145"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="862"/>
        <source>Decrypting and Verifying</source>
        <translation>Odszyfrowywanie i weryfikowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="301"/>
        <source>Missing Keys</source>
        <translation>Brakujące klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="302"/>
        <source>Some signatures cannot be verified because the corresponding keys are missing.

The following fingerprints are missing:
%1

Would you like to fetch these keys from the key server?</source>
        <translation>Nie można zweryfikować niektórych podpisów, ponieważ brakuje odpowiadających im kluczy.

Brakujące odciski kluczy:
%1

Czy chcesz pobrać te klucze z serwera kluczy?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="323"/>
        <source>Verification Incomplete</source>
        <translation>Weryfikacja niekompletna</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="324"/>
        <source>Verification was incomplete due to missing keys. You can manually import the keys later.</source>
        <translation>Weryfikacja była niekompletna z powodu brakujących kluczy. Możesz później ręcznie zaimportować te klucze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="131"/>
        <source>Localize</source>
        <translation>Czas lokalny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="134"/>
        <source>UTC</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="145"/>
        <source>Local Time</source>
        <translation>Czas lokalny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="369"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="346"/>
        <source>No operation result available.</source>
        <translation>Brak dostępnego wyniku operacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="365"/>
        <source>FAILED</source>
        <translation>NIEPOWODZENIE</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="372"/>
        <source>WARNING</source>
        <translation>OSTRZEŻENIE</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="378"/>
        <source>Object: %1</source>
        <translation>Obiekt: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="394"/>
        <source>Summary</source>
        <translation>Podsumowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="396"/>
        <source>Total operations: %1</source>
        <translation>Wszystkie operacje: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="402"/>
        <source>Failed objects: %1</source>
        <translation>Nieudane obiekty: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="406"/>
        <source>Warning objects: %1</source>
        <translation>Obiekty z ostrzeżeniem: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="411"/>
        <source>Details</source>
        <translation>Szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="474"/>
        <source>Email Operation Failed</source>
        <translation>Operacja e-mail nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="480"/>
        <source>The operation could not be completed. See the reason and suggested solutions below.</source>
        <translation>Nie udało się ukończyć operacji. Zobacz powód i proponowane rozwiązania poniżej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="482"/>
        <source>Report</source>
        <translation>Zgłoś</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="519"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="575"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="623"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="687"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="759"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="851"/>
        <source>Unsupported Operation</source>
        <translation>Nieobsługiwana operacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="520"/>
        <source>The decrypt operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Operacja odszyfrowywania dla typu karty „%1” nie jest obsługiwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="576"/>
        <source>The verify operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Operacja weryfikacji dla typu karty „%1” nie jest obsługiwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="624"/>
        <source>The encryption operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Operacja szyfrowania dla typu karty „%1” nie jest obsługiwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="631"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="697"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="769"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="788"/>
        <source>No Key Selected</source>
        <translation>Nie wybrano klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="632"/>
        <source>Please select a key for encryption.</source>
        <translation>Wybierz klucz do szyfrowania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="688"/>
        <source>The sign operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Operacja podpisywania dla typu karty „%1” nie jest obsługiwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="760"/>
        <source>The encrypt and sign operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Operacja szyfrowania i podpisywania dla typu karty „%1” nie jest obsługiwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="770"/>
        <source>Please select a key for encrypt the EML.</source>
        <translation>Wybierz klucz do zaszyfrowania pliku EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="852"/>
        <source>The decrypt and verify operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Operacja odszyfrowywania i weryfikacji dla typu karty „%1” nie jest obsługiwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="698"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="789"/>
        <source>Please select a key for signing the EML.</source>
        <translation>Wybierz klucz do podpisania pliku EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="397"/>
        <source>Successful: %1</source>
        <translation>Pomyślne: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="398"/>
        <source>Warnings: %1</source>
        <translation>Ostrzeżenia: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="399"/>
        <source>Failures: %1</source>
        <translation>Niepowodzenia: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="703"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="794"/>
        <source>Multiple Keys Selected</source>
        <translation>Wybrano wiele kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="704"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="795"/>
        <source>Please select only one key to sign the EML.</source>
        <translation>Wybierz tylko jeden klucz do podpisania pliku EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="78"/>
        <source>Save File</source>
        <translation>Zapisz plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="79"/>
        <source>Save the current File</source>
        <translation>Zapisz bieżący plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="83"/>
        <source>Save As</source>
        <translation>Zapisz jako</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="84"/>
        <source>Save the current File as...</source>
        <translation>Zapisz bieżący plik jako...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="87"/>
        <source>Print</source>
        <translation>Drukuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="88"/>
        <source>Print Document</source>
        <translation>Wydrukuj dokument</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="151"/>
        <source>Quit</source>
        <translation>Zakończ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="152"/>
        <source>Quit Program</source>
        <translation>Zakończ program</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="156"/>
        <source>Undo</source>
        <translation>Cofnij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="157"/>
        <source>Undo Last Edit Action</source>
        <translation>Cofnij ostatnią czynność edycji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="160"/>
        <source>Redo</source>
        <translation>Ponów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="161"/>
        <source>Redo Last Edit Action</source>
        <translation>Ponów ostatnią czynność edycji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="164"/>
        <source>Zoom In</source>
        <translation>Powiększ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="165"/>
        <source>Zoom in</source>
        <translation>Powiększ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="169"/>
        <source>Zoom Out</source>
        <translation>Pomniejsz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="170"/>
        <source>Zoom out</source>
        <translation>Pomniejsz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="174"/>
        <source>Paste</source>
        <translation>Wklej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="175"/>
        <source>Paste Text From Clipboard</source>
        <translation>Wklej tekst ze schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="179"/>
        <source>Cut</source>
        <translation>Wytnij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="180"/>
        <source>Cut the current selection&apos;s contents to the clipboard</source>
        <translation>Wytnij zawartość bieżącego zaznaczenia do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="185"/>
        <source>Copy</source>
        <translation>Skopiuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="186"/>
        <source>Copy the current selection&apos;s contents to the clipboard</source>
        <translation>Skopiuj zawartość bieżącego zaznaczenia do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="190"/>
        <source>Quote</source>
        <translation>Cytuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="191"/>
        <source>Quote whole text</source>
        <translation>Zacytuj cały tekst</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="195"/>
        <source>Select All</source>
        <translation>Zaznacz wszystko</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="196"/>
        <source>Select the whole text</source>
        <translation>Zaznacz cały tekst</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="200"/>
        <source>Find</source>
        <translation>Znajdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="201"/>
        <source>Find a word</source>
        <translation>Znajdź słowo</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="205"/>
        <source>Remove spacing</source>
        <translation>Usuń odstępy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="207"/>
        <source>Remove double linebreaks, e.g. in pasted text from Web Mailer</source>
        <translation>Usuń podwójne łamania linii, np. w tekście wklejonym z webowego klienta poczty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="212"/>
        <source>Settings</source>
        <translation>Ustawienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="213"/>
        <source>Open settings dialog</source>
        <translation>Otwórz okno ustawień</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="749"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="221"/>
        <source>Encrypt</source>
        <translation>Zaszyfruj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="222"/>
        <source>Encrypt Message</source>
        <translation>Zaszyfruj wiadomość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="229"/>
        <source>Encrypt and Sign Message</source>
        <translation>Zaszyfruj i podpisz wiadomość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="833"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="234"/>
        <source>Decrypt</source>
        <translation>Odszyfruj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="235"/>
        <source>Decrypt Message</source>
        <translation>Odszyfruj wiadomość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="242"/>
        <source>Decrypt and Verify Message</source>
        <translation>Odszyfruj i zweryfikuj wiadomość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="247"/>
        <source>Sign</source>
        <translation>Podpisz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="248"/>
        <source>Sign Message</source>
        <translation>Podpisz wiadomość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="252"/>
        <source>Verify</source>
        <translation>Zweryfikuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="253"/>
        <source>Verify Message</source>
        <translation>Zweryfikuj wiadomość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="259"/>
        <source>Sym. Encrypt</source>
        <translation>Zaszyfruj symetrycznie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="260"/>
        <source>Encrypt Message (Symmetric)</source>
        <translation>Zaszyfruj wiadomość (symetrycznie)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="270"/>
        <source>New Keypair</source>
        <translation>Nowa para kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="271"/>
        <source>Generate KeyPair</source>
        <translation>Wygeneruj parę kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="278"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="430"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="587"/>
        <source>File</source>
        <translation>Plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="280"/>
        <source>Import New Key From File</source>
        <translation>Importuj nowy klucz z pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="286"/>
        <source>Clipboard</source>
        <translation>Schowek</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="288"/>
        <source>Import New Key From Clipboard</source>
        <translation>Importuj nowy klucz ze schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="294"/>
        <source>Editor</source>
        <translation>Edytor</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="295"/>
        <source>Import New Key From Editor</source>
        <translation>Importuj nowy klucz z edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="300"/>
        <source>Manage Keys</source>
        <translation>Zarządzaj kluczami</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="301"/>
        <source>Open Key Management</source>
        <translation>Otwórz zarządzanie kluczami</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="312"/>
        <source>Open Smart Card Controller</source>
        <translation>Otwórz kontroler kart inteligentnych</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="313"/>
        <source>Open Smart Card Controller Dialog</source>
        <translation>Otwórz okno kontrolera kart inteligentnych</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="337"/>
        <source>Show Application Log</source>
        <translation>Pokaż dziennik aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="338"/>
        <source>Show the application log view</source>
        <translation>Pokaż widok dziennika aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="411"/>
        <source>IM Encrypt</source>
        <translation>Zaszyfruj do komunikatora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="412"/>
        <source>Encrypt the current text into a compact, single-line format that is safe to paste into instant messengers. Recipients decrypt it with the normal Decrypt action.</source>
        <translation>Szyfruje bieżący tekst do zwięzłego, jednoliniowego formatu, który można bezpiecznie wklejać do komunikatorów. Odbiorcy odszyfrowują go zwykłą czynnością Odszyfruj.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="420"/>
        <source>IM Encrypt &amp;&amp; Sign</source>
        <translation>Zaszyfruj i podpisz do komunikatora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="421"/>
        <source>Encrypt and sign the current text into a compact, single-line format that is safe to paste into instant messengers. Recipients decrypt and verify it with the normal Decrypt &amp; Verify action.</source>
        <translation>Szyfruje i podpisuje bieżący tekst do zwięzłego, jednoliniowego formatu, który można bezpiecznie wklejać do komunikatorów. Odbiorcy odszyfrowują i weryfikują go zwykłą czynnością Odszyfruj i zweryfikuj.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="432"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="593"/>
        <source>Open</source>
        <translation>Otwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="437"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="599"/>
        <source>Workspace</source>
        <translation>Obszar roboczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="537"/>
        <source>Advanced</source>
        <translation>Zaawansowane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="593"/>
        <source>Open a file or directory</source>
        <translation>Otwórz plik lub katalog</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="600"/>
        <source>Open a text editor or file panel</source>
        <translation>Otwórz edytor tekstu lub panel plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="620"/>
        <source>Import</source>
        <translation>Importuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="621"/>
        <source>Import a key from file, editor, or clipboard</source>
        <translation>Importuj klucz z pliku, edytora lub schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="639"/>
        <source>Text Tools</source>
        <translation>Narzędzia tekstowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="782"/>
        <source>Status Panel</source>
        <translation>Panel statusu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="150"/>
        <source>Open Profile File</source>
        <translation>Otwórz plik profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="187"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="247"/>
        <source>Cannot Open Profile</source>
        <translation>Nie można otworzyć profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="225"/>
        <source>Nothing opened yet</source>
        <translation>Nic jeszcze nie otwarto</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="238"/>
        <source>%1  (open in another window)</source>
        <translation>%1  (otwarty w innym oknie)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="314"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="333"/>
        <source>Cannot Export Profile</source>
        <translation>Nie można wyeksportować profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="315"/>
        <source>The application key is not available, so the profile could not be packed.</source>
        <translation>Klucz aplikacji jest niedostępny, więc nie udało się spakować profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="323"/>
        <source>Exporting Profile</source>
        <translation>Eksportowanie profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="342"/>
        <source>&quot;%1&quot; was written to a single file.</source>
        <translation>„%1” zapisano do pojedynczego pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="354"/>
        <source>Left out</source>
        <translation>Pominięto</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="356"/>
        <source>A profile file only carries the profile itself.</source>
        <translation>Plik profilu zawiera tylko sam profil.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="364"/>
        <source>Done</source>
        <translation>Gotowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="341"/>
        <source>Profile Exported</source>
        <translation>Profil wyeksportowany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="362"/>
        <source>It can only be opened with the passphrase you chose. There is no way to recover it.</source>
        <translation>Można go otworzyć tylko hasłem, które wybrano. Nie ma możliwości jego odzyskania.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="55"/>
        <source>New Text Editor</source>
        <translation>Nowy edytor tekstu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="61"/>
        <source>New File Panel</source>
        <translation>Nowy panel plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="67"/>
        <source>File...</source>
        <translation>Plik...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="68"/>
        <source>Open a file in the file panel</source>
        <translation>Otwórz plik w panelu plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="73"/>
        <source>Directory...</source>
        <translation>Katalog...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="74"/>
        <source>Open a directory in the file panel</source>
        <translation>Otwórz katalog w panelu plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="92"/>
        <source>Close Tab</source>
        <translation>Zamknij kartę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="93"/>
        <source>Close the current tab</source>
        <translation>Zamknij bieżącą kartę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="104"/>
        <source>Manage Profiles...</source>
        <translation>Zarządzaj profilami...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="105"/>
        <source>See every profile on this computer, and open, rename or remove one</source>
        <translation>Zobacz każdy profil na tym komputerze oraz otwórz, zmień nazwę lub usuń</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="113"/>
        <source>New Profile...</source>
        <translation>Nowy profil...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="114"/>
        <source>Start an empty profile on this computer, with its own settings and keys</source>
        <translation>Uruchom pusty profil na tym komputerze, z własnymi ustawieniami i kluczami</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="125"/>
        <source>Open Profile File...</source>
        <translation>Otwórz plik profilu...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="126"/>
        <source>Work inside a profile file, leaving it a file. Nothing is added to this computer, and your changes go back into the same file.</source>
        <translation>Pracuj wewnątrz pliku profilu, pozostawiając go plikiem. Na ten komputer nic nie jest dodawane, a Twoje zmiany wracają do tego samego pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="137"/>
        <source>Import Profile File...</source>
        <translation>Importuj plik profilu...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="138"/>
        <source>Copy a profile file into a new profile kept on this computer. The file is not used again afterwards.</source>
        <translation>Skopiuj plik profilu do nowego profilu przechowywanego na tym komputerze. Plik nie będzie już później używany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="144"/>
        <source>Export This Profile...</source>
        <translation>Eksportuj ten profil...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="145"/>
        <source>Write the profile this window is using out to a new profile file, to carry elsewhere or keep as a backup</source>
        <translation>Zapisz profil, z którego korzysta to okno, do nowego pliku profilu, aby przenieść gdzie indziej lub zachować jako kopię zapasową</translation>
    </message>
    <message>
        <source>Right-to-Left</source>
        <translation type="vanished">Od prawej do lewej</translation>
    </message>
    <message>
        <source>Lay this tab&apos;s text out from right to left, the way Arabic, Hebrew and Persian read</source>
        <translation type="vanished">Układaj tekst tej karty od prawej do lewej, tak jak czyta się arabski, hebrajski i perski</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="228"/>
        <source>Encrypt &amp;&amp; Sign</source>
        <translation>Zaszyfruj i podpisz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="241"/>
        <source>Decrypt &amp;&amp; Verify</source>
        <translation>Odszyfruj i zweryfikuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="306"/>
        <source>Open Module Controller</source>
        <translation>Otwórz kontroler modułów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="307"/>
        <source>Open Module Controller Dialog</source>
        <translation>Otwórz okno kontrolera modułów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="320"/>
        <source>About</source>
        <translation>O programie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="321"/>
        <source>Show the application&apos;s About box</source>
        <translation>Pokaż okno „O programie”</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="331"/>
        <source>Open Wizard</source>
        <translation>Otwórz kreator</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="332"/>
        <source>Open the wizard</source>
        <translation>Otwórz kreator</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="343"/>
        <source>Append Public Key to Editor</source>
        <translation>Dodaj klucz publiczny do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="344"/>
        <source>Append selected Keypair&apos;s Public Key to Editor</source>
        <translation>Dodaj klucz publiczny wybranej pary kluczy do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="349"/>
        <source>Append Create DateTime to Editor</source>
        <translation>Dodaj datę i godzinę utworzenia do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="350"/>
        <source>Append selected Key&apos;s creation date and time to Editor</source>
        <translation>Dodaj datę i godzinę utworzenia wybranego klucza do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="355"/>
        <source>Append Expire DateTime to Editor</source>
        <translation>Dodaj datę i godzinę wygaśnięcia do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="356"/>
        <source>Append selected Key&apos;s expiration date and time to Editor</source>
        <translation>Dodaj datę i godzinę wygaśnięcia wybranego klucza do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="361"/>
        <source>Append Fingerprint to Editor</source>
        <translation>Dodaj odcisk klucza do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="362"/>
        <source>Append selected Key&apos;s Fingerprint to Editor</source>
        <translation>Dodaj odcisk wybranego klucza do edytora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="367"/>
        <source>Copy Email</source>
        <translation>Skopiuj adres e-mail</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="368"/>
        <source>Copy selected Keypair&apos;s to clipboard</source>
        <translation>Skopiuj adres e-mail wybranej pary kluczy do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="373"/>
        <source>Copy Default UID</source>
        <translation>Skopiuj domyślny identyfikator użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="374"/>
        <source>Copy selected Keypair&apos;s default UID to clipboard</source>
        <translation>Skopiuj domyślny identyfikator użytkownika wybranej pary kluczy do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="379"/>
        <source>Copy Key ID</source>
        <translation>Skopiuj identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="380"/>
        <source>Copy selected Keypair&apos;s ID to clipboard</source>
        <translation>Skopiuj identyfikator wybranej pary kluczy do schowka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="385"/>
        <source>Show Key Details</source>
        <translation>Pokaż szczegóły klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="386"/>
        <source>Show Details for this Key</source>
        <translation>Pokaż szczegóły tego klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="391"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="392"/>
        <source>Set Owner Trust Level</source>
        <translation>Ustaw poziom zaufania do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="452"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="630"/>
        <source>Edit</source>
        <translation>Edytuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="483"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="614"/>
        <source>Keys</source>
        <translation>Klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="485"/>
        <source>Import Key</source>
        <translation>Importuj klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="514"/>
        <source>Profiles</source>
        <translation>Profile</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="519"/>
        <source>Open Recent</source>
        <translation>Otwórz ostatni</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="552"/>
        <source>View</source>
        <translation>Widok</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="562"/>
        <source>Help</source>
        <translation>Pomoc</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="470"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="609"/>
        <source>Operations</source>
        <translation>Operacje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="56"/>
        <source>Open a new text editor</source>
        <translation>Otwórz nowy edytor tekstu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="62"/>
        <source>Open a new file panel</source>
        <translation>Otwórz nowy panel plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="705"/>
        <source>Ready</source>
        <translation>Gotowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="709"/>
        <source>Key ToolBox</source>
        <translation>Przybornik kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="734"/>
        <source>Default</source>
        <translation>Domyślne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="742"/>
        <source>Key Group</source>
        <translation>Grupa kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="749"/>
        <source>Only Public Key</source>
        <translation>Tylko klucz publiczny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="757"/>
        <source>Has Private Key</source>
        <translation>Ma klucz prywatny</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleControllerDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="118"/>
        <source>Show Mods Directory</source>
        <translation>Pokaż katalog modułów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="87"/>
        <source>Registered Modules</source>
        <translation>Zarejestrowane moduły</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="88"/>
        <source>Global Register Table</source>
        <translation>Globalna tablica rejestru</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="89"/>
        <source>Debugger</source>
        <translation>Debugger</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="85"/>
        <source>Module Controller</source>
        <translation>Kontroler modułów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="106"/>
        <source>ID</source>
        <translation>Identyfikator</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="91"/>
        <source>Search modules...</source>
        <translation>Szukaj modułów...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="92"/>
        <source>All</source>
        <translation>Wszystkie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="96"/>
        <source>Inactive</source>
        <translation>Nieaktywne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="98"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="294"/>
        <source>Integrated</source>
        <translation>Wbudowane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="100"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="294"/>
        <source>External</source>
        <translation>Zewnętrzne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="104"/>
        <source>Select a module to see its details.</source>
        <translation>Wybierz moduł, aby zobaczyć jego szczegóły.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="107"/>
        <source>SDK Version</source>
        <translation>Wersja SDK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="108"/>
        <source>Qt ENV Version</source>
        <translation>Wersja środowiska Qt</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="109"/>
        <source>Hash</source>
        <translation>Skrót</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="110"/>
        <source>Path</source>
        <translation>Ścieżka</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="112"/>
        <source>Listening Events</source>
        <translation>Nasłuchiwane zdarzenia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="114"/>
        <source>Activate on Start</source>
        <translation>Aktywuj przy starcie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="116"/>
        <source>Activate this module automatically when GpgFrontend starts.</source>
        <translation>Aktywuj ten moduł automatycznie po uruchomieniu GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="117"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="123"/>
        <source>Refresh</source>
        <translation>Odśwież</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="120"/>
        <source>Search keys and values...</source>
        <translation>Szukaj kluczy i wartości...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="121"/>
        <source>Expand All</source>
        <translation>Rozwiń wszystkie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="122"/>
        <source>Collapse All</source>
        <translation>Zwiń wszystkie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="125"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="202"/>
        <source>Trigger Event</source>
        <translation>Wywołaj zdarzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="126"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="211"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="215"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="219"/>
        <source>Upsert GRT Value</source>
        <translation>Wstaw/aktualizuj wartość GRT</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="136"/>
        <source>%1 modules · %2 active</source>
        <translation>Moduły: %1 · aktywne: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="203"/>
        <source>Please provide an Event ID</source>
        <translation>Podaj identyfikator zdarzenia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="211"/>
        <source>Namespace</source>
        <translation>Przestrzeń nazw</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="215"/>
        <source>Key</source>
        <translation>Klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="220"/>
        <source>Value</source>
        <translation>Wartość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="236"/>
        <source>Module loading is disabled. Enable it in Settings &gt; General to use modules.</source>
        <translation>Ładowanie modułów jest wyłączone. Włącz je w Ustawieniach &gt; Ogólne, aby korzystać z modułów.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="240"/>
        <source>Only integrated modules are loaded. To load external modules from the mods directory, change the module loading policy in Settings &gt; General.</source>
        <translation>Ładowane są tylko moduły wbudowane. Aby ładować moduły zewnętrzne z katalogu mods, zmień zasady ładowania modułów w Ustawieniach &gt; Ogólne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="292"/>
        <source>● Active</source>
        <translation>● Aktywny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="292"/>
        <source>○ Inactive</source>
        <translation>○ Nieaktywny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="298"/>
        <source>Auto Start</source>
        <translation>Autostart</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="303"/>
        <source>by %1</source>
        <translation>autor: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="316"/>
        <source>N/A</source>
        <translation>n/d</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="327"/>
        <source>N/A (integrated)</source>
        <translation>n/d (wbudowany)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="331"/>
        <source>Click to open the containing folder</source>
        <translation>Kliknij, aby otworzyć folder zawierający</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="341"/>
        <source>Listening Events (%1)</source>
        <translation>Nasłuchiwane zdarzenia (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="94"/>
        <source>Active</source>
        <translation>Aktywne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="343"/>
        <source>Deactivate</source>
        <translation>Dezaktywuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="344"/>
        <source>Activate</source>
        <translation>Aktywuj</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleItemDelegate</name>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="196"/>
        <source>Integrated</source>
        <translation>Wbudowany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="196"/>
        <source>External</source>
        <translation>Zewnętrzny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="202"/>
        <source>Auto</source>
        <translation>Auto</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::MoveKeyToCardPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="120"/>
        <source>Select Key to Move to Card</source>
        <translation>Wybierz klucz do przeniesienia na kartę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="125"/>
        <source>Choose a Key or Subkey to Move</source>
        <translation>Wybierz klucz lub podklucz do przeniesienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="131"/>
        <source>Select the single private key or subkey to move onto the smart card. Expand a key to choose a particular subkey. Only parts that can be stored on a card are selectable.</source>
        <translation>Wybierz pojedynczy klucz prywatny lub podklucz do przeniesienia na kartę inteligentną. Rozwiń klucz, aby wybrać konkretny podklucz. Wybierać można tylko części, które da się przechować na karcie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="145"/>
        <source>Database</source>
        <translation>Baza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="175"/>
        <source>Confirm</source>
        <translation>Potwierdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="178"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="183"/>
        <source>Select One Key</source>
        <translation>Wybierz jeden klucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="184"/>
        <source>Please select exactly one key or subkey to move to the card.</source>
        <translation>Wybierz dokładnie jeden klucz lub podklucz do przeniesienia na kartę.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::NetworkTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="65"/>
        <source>Proxy</source>
        <translation>Serwer proxy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="66"/>
        <source>Network Ability</source>
        <translation>Możliwości sieciowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="67"/>
        <source>Operations</source>
        <translation>Operacje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="69"/>
        <source>Enable Proxy</source>
        <translation>Ustaw serwer proxy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="70"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="73"/>
        <source>Port</source>
        <translation>Port</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="72"/>
        <source>Host Address</source>
        <translation>Adres hosta</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="74"/>
        <source>Proxy Type</source>
        <translation>Typ proxy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="75"/>
        <source>Username</source>
        <translation>Nazwa użytkownika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="76"/>
        <source>Password</source>
        <translation>Hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="79"/>
        <source>Apply Proxy Settings and Check Proxy Connection</source>
        <translation>Zastosuj ustawienia proxy i sprawdź połączenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="81"/>
        <source>Automatically fetch key publish status from key server.</source>
        <translation>Automatycznie pobieraj status publikacji kluczy z serwera kluczy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="170"/>
        <source>Test Server Url Accessibility</source>
        <translation>Testuj dostępność adresu URL serwera</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="171"/>
        <source>Server Url</source>
        <translation>Adres URL serwera</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="181"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="182"/>
        <source>Successfully connect to the target server through the proxy server.</source>
        <translation>Pomyślnie połączono się z serwerem docelowym przez serwer proxy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="186"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="187"/>
        <source>Unable to connect to the target server through the proxy server. Proxy settings may be invalid.</source>
        <translation>Nie można połączyć się z serwerem docelowym przez serwer proxy. Ustawienia proxy mogą być nieprawidłowe.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="197"/>
        <source>Test Proxy Server Connection...</source>
        <translation>Testuj połączenie z serwerem proxy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="198"/>
        <source>Is using your proxy settings to access the url. Note that this test operation will apply your proxy settings to the entire software.</source>
        <translation>Używa Twoich ustawień proxy, aby uzyskać dostęp do adresu URL. Ta operacja testowa zastosuje ustawienia proxy w całym programie.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::PassphraseDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="39"/>
        <source>Passphrase Required</source>
        <translation>Wymagane hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="48"/>
        <source>Enter Passphrase</source>
        <translation>Wprowadź hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="55"/>
        <source>Please enter the passphrase required for the current operation.</source>
        <translation>Wprowadź hasło wymagane dla bieżącej operacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="90"/>
        <source>Passphrase info: %1</source>
        <translation>Informacje o haśle: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="277"/>
        <source>Empty Passphrase</source>
        <translation>Puste hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="278"/>
        <source>Passphrase cannot be empty. Please enter a valid passphrase.</source>
        <translation>Hasło nie może być puste. Wprowadź prawidłowe hasło.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="287"/>
        <source>Passphrase Mismatch</source>
        <translation>Niezgodność haseł</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="288"/>
        <source>The two passphrases do not match. Please enter them again.</source>
        <translation>Hasła nie są zgodne. Wprowadź je ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="117"/>
        <source>Enter your passphrase here</source>
        <translation>Wprowadź tutaj swoje hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="64"/>
        <source>Incorrect Passphrase</source>
        <translation>Nieprawidłowe hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="67"/>
        <source>The passphrase you entered was incorrect. Please try again.</source>
        <translation>Wprowadzone hasło było nieprawidłowe. Spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="94"/>
        <source>This passphrase will be used to set a new password.</source>
        <translation>To hasło zostanie użyte do ustawienia nowego hasła.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="99"/>
        <source>Key ID: %1</source>
        <translation>Identyfikator klucza: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="100"/>
        <source>Key UID: %1</source>
        <translation>Identyfikator użytkownika klucza: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="124"/>
        <source>Enter your passphrase again</source>
        <translation>Wprowadź swoje hasło ponownie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="134"/>
        <source>Show</source>
        <translation>Pokaż</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="154"/>
        <source>Passphrase:</source>
        <translation>Hasło:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="174"/>
        <source>Strength:</source>
        <translation>Siła:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="182"/>
        <source>Confirm:</source>
        <translation>Potwierdź:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="195"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="317"/>
        <source>Closing in %1:%2</source>
        <translation>Zamknięcie za %1:%2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="194"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::PlainTextEditorPage</name>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="137"/>
        <source>Number of characters in the editor.</source>
        <translation>Liczba znaków w edytorze.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="139"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="224"/>
        <source>LF</source>
        <translation>LF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="140"/>
        <source>Line ending style.</source>
        <translation>Styl końca linii.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="144"/>
        <source>Text encoding.</source>
        <translation>Kodowanie tekstu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="154"/>
        <source>SHA-256 checksum of editor content.</source>
        <translation>Suma kontrolna SHA-256 zawartości edytora.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="218"/>
        <source>Ln %1, Col %2 · %3 chars%4</source>
        <translation>Lin %1, Kol %2 · %3 znaków%4</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="224"/>
        <source>CRLF</source>
        <translation>CRLF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="230"/>
        <source>The document has unsaved changes.</source>
        <translation>Dokument ma niezapisane zmiany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="231"/>
        <source>The document is unchanged.</source>
        <translation>Dokument jest niezmieniony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="449"/>
        <source>Loading... %1 KB</source>
        <translation>Wczytywanie... %1 KB</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="771"/>
        <source>Message</source>
        <translation>Wiadomość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="772"/>
        <source>Raw Source</source>
        <translation>Surowe źródło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="881"/>
        <source>Text Direction</source>
        <translation>Kierunek tekstu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="888"/>
        <source>Automatic</source>
        <translation>Automatyczny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="889"/>
        <source>Left-to-Right</source>
        <translation>Od lewej do prawej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="890"/>
        <source>Right-to-Left</source>
        <translation>Od prawej do lewej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="904"/>
        <source>Which way the text runs. Automatic gives every line the direction of its own first letter.</source>
        <translation>W którą stronę biegnie tekst. Tryb automatyczny nadaje każdemu wierszowi kierunek jego pierwszej litery.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="91"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="134"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="200"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="379"/>
        <source>Loading...</source>
        <translation>Wczytywanie...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="136"/>
        <source>Ln 1, Col 1 · 0 chars</source>
        <translation>Lin 1, Kol 1 · 0 znaków</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="143"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="225"/>
        <source>UTF-8</source>
        <translation>UTF-8</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileCreateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="65"/>
        <source>New Profile</source>
        <translation>Nowy profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="87"/>
        <source>for example: Work</source>
        <translation>na przykład: Praca</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="88"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="91"/>
        <source>Keys</source>
        <translation>Klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="94"/>
        <source>Start with an empty keyring</source>
        <translation>Zacznij z pustą bazą kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="97"/>
        <source>Keys live inside this profile. It stays separate from everything else and can be exported as a single file. It opens with no keys, so you will import or create them here.</source>
        <translation>Klucze żyją wewnątrz tego profilu. Pozostaje on oddzielony od wszystkiego innego i można go wyeksportować jako pojedynczy plik. Otwiera się bez kluczy, więc zaimportujesz lub utworzysz je tutaj.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="103"/>
        <source>Use the system GnuPG keyring</source>
        <translation>Użyj systemowej bazy kluczy GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="105"/>
        <source>The same keys you already have. They are shared with the rest of the system, so they are not really separate and cannot be carried inside a profile file.</source>
        <translation>Te same klucze, które już masz. Są wspólne z resztą systemu, więc nie są naprawdę oddzielne i nie mogą być przenoszone w pliku profilu.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileExportDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="52"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="231"/>
        <source>Export Profile</source>
        <translation>Eksportuj profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="150"/>
        <source>No file chosen yet.</source>
        <translation>Nie wybrano jeszcze pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="159"/>
        <source>Choose...</source>
        <translation>Wybierz...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="165"/>
        <source>Save to</source>
        <translation>Zapisz do</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="195"/>
        <source>The file cannot be opened without this passphrase, and it cannot be recovered.</source>
        <translation>Tego pliku nie można otworzyć bez tego hasła i nie da się go odzyskać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="74"/>
        <source>Export &quot;%1&quot;</source>
        <translation>Eksportuj „%1”</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="75"/>
        <source>Everything below is written into a single file you can copy to another computer and import there.</source>
        <translation>Wszystko poniżej zostanie zapisane do pojedynczego pliku, który możesz skopiować na inny komputer i tam zaimportować.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="176"/>
        <source>What goes in</source>
        <translation>Co zostanie uwzględnione</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="283"/>
        <source>Choose where to save the file to continue.</source>
        <translation>Wybierz, gdzie zapisać plik, aby kontynuować.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="210"/>
        <source>Protection</source>
        <translation>Ochrona</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="119"/>
        <source>Export</source>
        <translation>Eksportuj</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileManagerDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="72"/>
        <source>Profiles On This Computer</source>
        <translation>Profile na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="89"/>
        <source>The profiles kept on this computer. Each one has its own settings, keys and saved state. Opening one starts a new window; this window stays exactly as it is.</source>
        <translation>Profile przechowywane na tym komputerze. Każdy ma własne ustawienia, klucze i zapisany stan. Otwarcie uruchamia nowe okno; to okno pozostaje dokładnie takie, jakie jest.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Folder</source>
        <translation>Folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Last Opened</source>
        <translation>Ostatnio otwarty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="117"/>
        <source>Open</source>
        <translation>Otwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="119"/>
        <source>New...</source>
        <translation>Nowy...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="120"/>
        <source>Import...</source>
        <translation>Importuj...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="121"/>
        <source>Delete</source>
        <translation>Usuń</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="122"/>
        <source>Open Folder</source>
        <translation>Otwórz folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="124"/>
        <source>Open the selected profile in a new window</source>
        <translation>Otwórz wybrany profil w nowym oknie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="125"/>
        <source>Start an empty profile on this computer</source>
        <translation>Uruchom pusty profil na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="130"/>
        <source>Copy a profile file into a new profile kept on this computer. The file is not used again afterwards.</source>
        <translation>Skopiuj plik profilu do nowego profilu przechowywanego na tym komputerze. Plik nie będzie już później używany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="133"/>
        <source>Remove the selected profile and everything in it, permanently</source>
        <translation>Trwale usuń wybrany profil i wszystko, co się w nim znajduje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="135"/>
        <source>Show the selected profile&apos;s folder in the file manager</source>
        <translation>Pokaż folder wybranego profilu w menedżerze plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="144"/>
        <source>Close</source>
        <translation>Zamknij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="178"/>
        <source>%1  (open)</source>
        <translation>%1  (otwarty)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="198"/>
        <source>This window</source>
        <translation>To okno</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="200"/>
        <source>Not created yet</source>
        <translation>Jeszcze nie utworzony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="203"/>
        <source>Open in another window</source>
        <translation>Otwórz w innym oknie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="242"/>
        <source>Cannot Open Profile</source>
        <translation>Nie można otworzyć profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="258"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="267"/>
        <source>Delete Profile</source>
        <translation>Usuń profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="259"/>
        <source>Delete &quot;%1&quot;?</source>
        <translation>Usunąć „%1”?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="260"/>
        <source>Its keys, settings and saved state are removed from this computer permanently.</source>
        <translation>Jego klucze, ustawienia i zapisany stan zostaną trwale usunięte z tego komputera.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="268"/>
        <source>This cannot be undone. Anything stored only in &quot;%1&quot; will be lost.</source>
        <translation>Tej operacji nie można cofnąć. Wszystko zapisane wyłącznie w „%1” zostanie utracone.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="277"/>
        <source>Profile Is Open</source>
        <translation>Profil jest otwarty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="278"/>
        <source>&quot;%1&quot; is open in another window. Close it first.</source>
        <translation>Profil „%1” jest otwarty w innym oknie. Najpierw go zamknij.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="296"/>
        <source>Cannot Delete Profile</source>
        <translation>Nie można usunąć profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="297"/>
        <source>The profile folder could not be removed:</source>
        <translation>Nie udało się usunąć folderu profilu:</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::QuitDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="35"/>
        <source>Unsaved Files</source>
        <translation>Niezapisane pliki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="82"/>
        <source>%1 files contain unsaved information.&lt;br/&gt;Save the changes before closing?</source>
        <translation>Plików z niezapisanymi informacjami: %1.&lt;br/&gt;Zapisać zmiany przed zamknięciem?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="96"/>
        <source>Check the files you want to save:</source>
        <translation>Zaznacz pliki, które chcesz zapisać:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="98"/>
        <source>Note</source>
        <translation>Uwaga</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="99"/>
        <source>If you don&apos;t save these files, all changes are lost.</source>
        <translation>Jeśli nie zapiszesz tych plików, wszystkie zmiany przepadną.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::RevocationOptionsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/RevocationOptionsDialog.cpp" line="42"/>
        <source>Revocation Reason (Code)</source>
        <translation>Powód unieważnienia (kod)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/RevocationOptionsDialog.cpp" line="43"/>
        <source>Revocation Reason (Text)</source>
        <translation>Powód unieważnienia (tekst)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/RevocationOptionsDialog.cpp" line="44"/>
        <source>Revocation Options</source>
        <translation>Opcje unieważnienia</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::RpgpEngineTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="709"/>
        <source>Unknown</source>
        <translation>Nieznana</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="718"/>
        <source>GpgFrontend supports multiple OpenPGP backends. Alongside GnuPG, it can use a Rust-based engine (rPGP), giving you the freedom to choose the backend that best fits your needs. The details below describe the rPGP engine compiled into this build.</source>
        <translation>GpgFrontend obsługuje wiele silników OpenPGP. Obok GnuPG może korzystać z silnika opartego na Ruście (rPGP), dając Ci swobodę wyboru silnika najlepiej dopasowanego do Twoich potrzeb. Szczegóły poniżej opisują silnik rPGP skompilowany w tej wersji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="728"/>
        <source>Engine Version:</source>
        <translation>Wersja silnika:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="730"/>
        <source>Rust Compiler:</source>
        <translation>Kompilator Rust:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="733"/>
        <source>Target:</source>
        <translation>Cel:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="737"/>
        <source>Build Profile:</source>
        <translation>Profil kompilacji:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="742"/>
        <source>rPGP Engine</source>
        <translation>Silnik rPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="749"/>
        <source>Key Dependencies</source>
        <translation>Kluczowe zależności</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="768"/>
        <source>Copy Engine Information</source>
        <translation>Skopiuj informacje o silniku</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::RpgpTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="42"/>
        <source>Password Cache</source>
        <translation>Pamięć podręczna haseł</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="45"/>
        <source>Password Cache TTL (minutes):</source>
        <translation>TTL pamięci podręcznej haseł (minuty):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="47"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="54"/>
        <source> min</source>
        <translation> min</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="49"/>
        <source>Idle time the rPGP engine keeps an entered passphrase cached. The window is renewed each time the passphrase is used.</source>
        <translation>Czas bezczynności, przez który silnik rPGP przechowuje wprowadzone hasło w pamięci podręcznej. Okno jest odnawiane przy każdym użyciu hasła.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="52"/>
        <source>Password Cache Max TTL (minutes):</source>
        <translation>Maksymalny TTL pamięci podręcznej haseł (minuty):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="56"/>
        <source>Absolute lifetime of a cached passphrase, measured from when it was first entered, regardless of use. Never shorter than the TTL.</source>
        <translation>Bezwzględny czas życia hasła w pamięci podręcznej, liczony od jego pierwszego wprowadzenia, niezależnie od użycia. Nigdy krótszy niż TTL.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="60"/>
        <source>These options only apply to the rPGP engine&apos;s in-memory passphrase cache.</source>
        <translation>Te opcje dotyczą tylko pamięci podręcznej haseł silnika rPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="63"/>
        <source>Passphrase Prompt</source>
        <translation>Monit o hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="67"/>
        <source>Passphrase Prompt Timeout (seconds):</source>
        <translation>Limit czasu monitu o hasło (sekundy):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="69"/>
        <source> s</source>
        <translation> s</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="70"/>
        <source>Never</source>
        <translation>Nigdy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="72"/>
        <source>Time the passphrase prompt waits for an answer before it closes itself and cancels the operation. Set to 0 to let it wait indefinitely.</source>
        <translation>Czas, przez który monit o hasło czeka na odpowiedź, zanim sam się zamknie i anuluje operację. Ustaw 0, aby czekał bez ograniczeń.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="77"/>
        <source>Applies to the passphrase prompt GpgFrontend shows for the rPGP engine. GnuPG keys are unlocked through pinentry, which has its own timeout.</source>
        <translation>Dotyczy monitu o hasło, który GpgFrontend pokazuje dla silnika rPGP. Klucze GnuPG są odblokowywane przez pinentry, który ma własny limit czasu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="81"/>
        <source>Passphrase Encryption</source>
        <translation>Szyfrowanie hasłem</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="86"/>
        <source>Argon2 Parameters:</source>
        <translation>Parametry Argon2:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="87"/>
        <source>High memory, 2 GiB</source>
        <translation>Duża pamięć, 2 GiB</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="89"/>
        <source>Low memory, 64 MiB</source>
        <translation>Mała pamięć, 64 MiB</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="92"/>
        <source>How much work it takes to turn the passphrase into an encryption key. More memory makes a stolen message far more expensive to crack.</source>
        <translation>Określa, ile pracy wymaga przekształcenie hasła w klucz szyfrowania. Więcej pamięci sprawia, że złamanie skradzionej wiadomości jest znacznie bardziej kosztowne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="97"/>
        <source>Decrypting a message needs as much memory as encrypting it, on every machine that opens it. Choose the low-memory option if this computer or the recipient&apos;s cannot spare 2 GiB. This applies only to messages encrypted from now on. </source>
        <translation>Odszyfrowanie wiadomości wymaga takiej samej ilości pamięci jak jej zaszyfrowanie — na każdym komputerze, który ją otwiera. Wybierz opcję z małą ilością pamięci, jeśli ten komputer lub komputer odbiorcy nie może udostępnić 2 GiB. Dotyczy to wyłącznie wiadomości zaszyfrowanych od teraz. </translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SecretEntryPanel</name>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="95"/>
        <source>Generate</source>
        <translation>Wygeneruj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="98"/>
        <source>Invent a strong passphrase and show it, so you can write it down</source>
        <translation>Wymyśl silne hasło i pokaż je, żeby można było je zapisać</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="253"/>
        <source>A passphrase could not be generated.</source>
        <translation>Nie udało się wygenerować hasła.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="272"/>
        <source>Copied to the clipboard. Save it somewhere safe now — it is not stored anywhere, and it cannot be recovered later.</source>
        <translation>Skopiowano do schowka. Zapisz je teraz w bezpiecznym miejscu — nie jest nigdzie przechowywane i nie będzie można go później odzyskać.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SetOwnerTrustLevel</name>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="101"/>
        <source>Undefined</source>
        <translation>Nieokreślone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="99"/>
        <source>Never</source>
        <translation>Nigdy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="97"/>
        <source>Marginal</source>
        <translation>Marginalne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="95"/>
        <source>Full</source>
        <translation>Pełne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="48"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="93"/>
        <source>Ultimate</source>
        <translation>Ostateczne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="87"/>
        <source>Modify Owner Trust Level</source>
        <translation>Zmień poziom zaufania do właściciela</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="88"/>
        <source>Trust for the Key Pair:</source>
        <translation>Zaufanie dla pary kluczy:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="113"/>
        <source>Failed</source>
        <translation>Niepowodzenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="114"/>
        <source>Modify Owner Trust Level failed.</source>
        <translation>Zmiana poziomu zaufania do właściciela nie powiodła się.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SettingsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="92"/>
        <source>General</source>
        <translation>Ogólne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="93"/>
        <source>Appearance</source>
        <translation>Wygląd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="122"/>
        <source>Network</source>
        <translation>Sieć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="94"/>
        <source>Key Databases</source>
        <translation>Bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="85"/>
        <source>Application</source>
        <translation>Aplikacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="86"/>
        <source>Keys &amp; Engines</source>
        <translation>Klucze i silniki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="87"/>
        <source>Features</source>
        <translation>Funkcje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="88"/>
        <source>System</source>
        <translation>System</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="95"/>
        <source>GnuPG</source>
        <translation>GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="96"/>
        <source>Advanced</source>
        <translation>Zaawansowane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>startup</source>
        <translation>uruchamianie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>confirm import</source>
        <translation>potwierdzenie importu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>language</source>
        <translation>język</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>locale</source>
        <translation>lokalizacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="109"/>
        <source>translation</source>
        <translation>tłumaczenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="109"/>
        <source>data</source>
        <translation>dane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="109"/>
        <source>cache</source>
        <translation>pamięć podręczna</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>theme</source>
        <translation>motyw</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>icon</source>
        <translation>ikona</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>font size</source>
        <translation>rozmiar czcionki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>font family</source>
        <translation>krój czcionki</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="114"/>
        <source>toolbar</source>
        <translation>pasek narzędzi</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="114"/>
        <source>actions</source>
        <translation>akcje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="114"/>
        <source>instant messaging</source>
        <translation>komunikatory</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>text editor</source>
        <translation>edytor tekstu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>status panel</source>
        <translation>panel statusu</translation>
    </message>
    <message>
        <source>text direction</source>
        <translation type="vanished">kierunek tekstu</translation>
    </message>
    <message>
        <source>right-to-left</source>
        <translation type="vanished">od prawej do lewej</translation>
    </message>
    <message>
        <source>rtl</source>
        <translation type="vanished">rtl</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>arabic</source>
        <translation>arabski</translation>
    </message>
    <message>
        <source>hebrew</source>
        <translation type="vanished">hebrajski</translation>
    </message>
    <message>
        <source>persian</source>
        <translation type="vanished">perski</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>proxy</source>
        <translation>proxy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>socks</source>
        <translation>socks</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>http</source>
        <translation>http</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>timeout</source>
        <translation>limit czasu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="125"/>
        <source>connection</source>
        <translation>połączenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="131"/>
        <source>keyring</source>
        <translation>baza kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="131"/>
        <source>gpg home</source>
        <translation>katalog gpg</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="131"/>
        <source>database path</source>
        <translation>ścieżka bazy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="138"/>
        <source>gpgme</source>
        <translation>gpgme</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="138"/>
        <source>gpgconf</source>
        <translation>gpgconf</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="138"/>
        <source>binary path</source>
        <translation>ścieżka pliku binarnego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="138"/>
        <source>custom install</source>
        <translation>własna instalacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>password cache</source>
        <translation>pamięć podręczna haseł</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>reload</source>
        <translation>przeładowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>restart</source>
        <translation>ponowne uruchomienie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>gpg-agent</source>
        <translation>gpg-agent</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="140"/>
        <source>maintenance</source>
        <translation>konserwacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="145"/>
        <source>rPGP</source>
        <translation>rPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="145"/>
        <source>rust</source>
        <translation>rust</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="145"/>
        <source>engine</source>
        <translation>silnik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="150"/>
        <source>Instant Messaging</source>
        <translation>Komunikatory</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>message book</source>
        <translation>książka wiadomości</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>phrase</source>
        <translation>fraza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>fingerprint</source>
        <translation>odcisk</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>token</source>
        <translation>token</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="156"/>
        <source>security level</source>
        <translation>poziom bezpieczeństwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="156"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="156"/>
        <source>keychain</source>
        <translation>brelok kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="157"/>
        <source>log level</source>
        <translation>poziom dziennika</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="157"/>
        <source>ring buffer</source>
        <translation>bufor pierścieniowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="157"/>
        <source>ENV.ini</source>
        <translation>ENV.ini</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="180"/>
        <source>Search settings…</source>
        <translation>Szukaj w ustawieniach…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="233"/>
        <source>Settings</source>
        <translation>Ustawienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="468"/>
        <source>Restart Required</source>
        <translation>Wymagane ponowne uruchomienie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="469"/>
        <source>Some of your changes only take effect after GpgFrontend restarts.</source>
        <translation>Niektóre zmiany zadziałają dopiero po ponownym uruchomieniu GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="471"/>
        <source>Some of your changes only take effect after the interface reloads.</source>
        <translation>Niektóre zmiany zadziałają dopiero po przeładowaniu interfejsu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="474"/>
        <source>Changes needing this were made on: %1.

Choose Cancel to discard everything you changed in this dialog and keep the current settings.</source>
        <translation>Zmiany wymagające tego wprowadzono w: %1.

Wybierz Anuluj, aby odrzucić wszystkie zmiany z tego okna i zachować bieżące ustawienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="479"/>
        <source>Save and Restart</source>
        <translation>Zapisz i uruchom ponownie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="479"/>
        <source>Save and Reload</source>
        <translation>Zapisz i przeładuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="570"/>
        <source>System Default</source>
        <translation>Domyślny systemowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="223"/>
        <source>Preference</source>
        <translation>Preferencje</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SigningKeysPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="81"/>
        <source>Select Signing Key(s)</source>
        <translation>Wybierz klucze do podpisywania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="86"/>
        <source>Choose Signing Key(s) or Subkey(s)</source>
        <translation>Wybierz klucz lub podklucz do podpisywania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="92"/>
        <source>Select the private key(s) or specific signing subkey(s) to use for this operation. Expand a key to choose a particular subkey.</source>
        <translation>Wybierz klucze prywatne lub konkretne podklucze podpisujące do użycia w tej operacji. Rozwiń klucz, aby wybrać konkretny podklucz.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="106"/>
        <source>Confirm</source>
        <translation>Potwierdź</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="109"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="114"/>
        <source>No Key Selected</source>
        <translation>Nie wybrano klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="115"/>
        <source>Please select at least one signing key or subkey.</source>
        <translation>Wybierz co najmniej jeden klucz lub podklucz do podpisywania.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SmartCardControllerDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="158"/>
        <source>Change Name</source>
        <translation>Zmień nazwę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="160"/>
        <source>Change Language</source>
        <translation>Zmień język</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="164"/>
        <source>Change Login Data</source>
        <translation>Zmień dane logowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="166"/>
        <source>Change Public Key URL</source>
        <translation>Zmień adres URL klucza publicznego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="171"/>
        <source>Change PIN</source>
        <translation>Zmień PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="173"/>
        <source>Change Admin PIN</source>
        <translation>Zmień PIN administratora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="175"/>
        <source>Change Reset Code</source>
        <translation>Zmień kod resetowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="141"/>
        <source>Fetch</source>
        <translation>Pobierz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="118"/>
        <source>Slot</source>
        <translation>Slot</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="127"/>
        <source>Key Info (ki)</source>
        <translation>Informacje o kluczu (ki)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="128"/>
        <source>Additional Auth (aac)</source>
        <translation>Dodatkowe uwierzytelnianie (aac)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="129"/>
        <source>Biometric Terminal (bt)</source>
        <translation>Terminal biometryczny (bt)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="130"/>
        <source>KDF Supported</source>
        <translation>Obsługa KDF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="180"/>
        <source>Restart All Gpg-Agents</source>
        <translation>Uruchom ponownie wszystkie agenty GPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="139"/>
        <source>Generate Card Keys</source>
        <translation>Wygeneruj klucze karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="142"/>
        <source>Refresh</source>
        <translation>Odśwież</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="194"/>
        <source>Successful Operation</source>
        <translation>Operacja zakończona pomyślnie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="195"/>
        <source>Restart all the GnuPG&apos;s components successfully</source>
        <translation>Pomyślnie ponownie uruchomiono wszystkie składniki GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="198"/>
        <source>Failed Operation</source>
        <translation>Operacja nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="199"/>
        <source>Failed to restart all or one of the GnuPG&apos;s component(s)</source>
        <translation>Nie udało się ponownie uruchomić wszystkich lub jednego ze składników GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="229"/>
        <source>Generate card key failed.</source>
        <translation>Generowanie klucza karty nie powiodło się.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="97"/>
        <source>Smart Card Controller</source>
        <translation>Kontroler kart inteligentnych</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="102"/>
        <source>Reader</source>
        <translation>Czytnik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="103"/>
        <source>Serial Number</source>
        <translation>Numer seryjny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="104"/>
        <source>Manufacturer</source>
        <translation>Producent</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="107"/>
        <source>Language</source>
        <translation>Język</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="108"/>
        <source>Sex</source>
        <translation>Płeć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="111"/>
        <source>Signature Counter</source>
        <translation>Licznik podpisów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="112"/>
        <source>CHV1 Cached</source>
        <translation>CHV1 w pamięci podręcznej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="113"/>
        <source>KDF Status</source>
        <translation>Status KDF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="350"/>
        <source>Enabled</source>
        <translation>Włączone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="351"/>
        <source>Disabled</source>
        <translation>Wyłączone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="131"/>
        <source>Status Indicator</source>
        <translation>Wskaźnik statusu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="135"/>
        <source>No card detected</source>
        <translation>Nie wykryto karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="137"/>
        <source>Cardholder</source>
        <translation>Posiadacz karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="138"/>
        <source>Access Codes</source>
        <translation>Kody dostępu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="140"/>
        <source>Move Key to Card</source>
        <translation>Przenieś klucz na kartę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="143"/>
        <source>More Actions</source>
        <translation>Więcej czynności</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="162"/>
        <source>Change Sex</source>
        <translation>Zmień płeć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="203"/>
        <source>Open GnuPG Smart Card HOWTO</source>
        <translation>Otwórz podręcznik GnuPG Smart Card HOWTO</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="298"/>
        <source>Unnamed Card</source>
        <translation>Karta bez nazwy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="312"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="314"/>
        <source>%1, version %2</source>
        <translation>%1, wersja %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="317"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="318"/>
        <source>Not set</source>
        <translation>Nie ustawiono</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="321"/>
        <source>● Ready</source>
        <translation>● Gotowa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="333"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="333"/>
        <source>Reset Code</source>
        <translation>Kod resetowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="334"/>
        <source>Admin PIN</source>
        <translation>PIN administratora</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="340"/>
        <source>%1 n/a</source>
        <translation>%1: nie dotyczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="344"/>
        <source>%1 %2 left</source>
        <translation>%1: pozostało %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="347"/>
        <source>%1 retries left, maximum length %2</source>
        <translation>Pozostało prób: %1, maksymalna długość: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="353"/>
        <source>User Interaction Flag — Sign: %1 · Encrypt: %2 · Authenticate: %3</source>
        <translation>Flaga interakcji użytkownika — Podpis: %1 · Szyfrowanie: %2 · Uwierzytelnianie: %3</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="398"/>
        <source>N/A</source>
        <translation>brak</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="474"/>
        <source>Read the GnuPG Smart Card HOWTO</source>
        <translation>Przeczytaj podręcznik GnuPG Smart Card HOWTO</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="560"/>
        <source>No Card</source>
        <translation>Brak karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="561"/>
        <source>No smart card is currently selected.</source>
        <translation>Nie wybrano żadnej karty inteligentnej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="568"/>
        <source>Not Supported</source>
        <translation>Nieobsługiwane</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="569"/>
        <source>Moving a key to a smart card is only supported for GnuPG key databases. The rpgp engine is not supported for this feature.</source>
        <translation>Przenoszenie klucza na kartę inteligentną jest obsługiwane tylko dla baz kluczy GnuPG. Silnik rpgp nie obsługuje tej funkcji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="591"/>
        <source>Cardholder&apos;s Surname</source>
        <translation>Nazwisko posiadacza karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="592"/>
        <source>Please enter your surname (e.g., Lee):</source>
        <translation>Wprowadź swoje nazwisko (np. Kowalski):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="597"/>
        <source>Cardholder&apos;s Given Name</source>
        <translation>Imię posiadacza karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="598"/>
        <source>Please enter your given name (e.g., Chris):</source>
        <translation>Wprowadź swoje imię (np. Jan):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="607"/>
        <source>Too Long</source>
        <translation>Za długie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="608"/>
        <source>Combined name too long (max 39 characters).</source>
        <translation>Łączna nazwa jest za długa (maks. 39 znaków).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="623"/>
        <source>Male</source>
        <translation>Mężczyzna</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="623"/>
        <source>Female</source>
        <translation>Kobieta</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="627"/>
        <source>Select sex to store in &apos;%1&apos;</source>
        <translation>Wybierz płeć do zapisania w „%1”</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="642"/>
        <source>Enter new value for attribute &apos;%1&apos;</source>
        <translation>Wprowadź nową wartość atrybutu „%1”</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="364"/>
        <source>Not enabled</source>
        <translation>Nie włączone</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="367"/>
        <source>Enabled (no protection)</source>
        <translation>Włączone (bez ochrony)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="370"/>
        <source>Enabled with salt protection</source>
        <translation>Włączone z ochroną solą</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="305"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="373"/>
        <source>Unknown</source>
        <translation>Nieznany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="384"/>
        <source>No key information available.</source>
        <translation>Brak dostępnych informacji o kluczu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="119"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="118"/>
        <source>Created</source>
        <translation>Utworzono</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="119"/>
        <source>Grip</source>
        <translation>Uchwyt</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="118"/>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="99"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="105"/>
        <source>Card</source>
        <translation>Karta</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="101"/>
        <source>Identity</source>
        <translation>Tożsamość</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="106"/>
        <source>Application</source>
        <translation>Aplikacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="110"/>
        <source>Access &amp; Status</source>
        <translation>Dostęp i status</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="115"/>
        <source>Keys on Card</source>
        <translation>Klucze na karcie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="118"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="118"/>
        <source>Usage</source>
        <translation>Zastosowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="126"/>
        <source>Extended Capabilities</source>
        <translation>Rozszerzone możliwości</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="359"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="413"/>
        <source>Yes</source>
        <translation>Tak</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="359"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="414"/>
        <source>No</source>
        <translation>Nie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="133"/>
        <source>Additional Info</source>
        <translation>Dodatkowe informacje</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="456"/>
        <source>No OpenPGP Smart Card Found</source>
        <translation>Nie znaleziono karty inteligentnej OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="461"/>
        <source>No OpenPGP-compatible smart card has been detected.</source>
        <translation>Nie wykryto żadnej karty inteligentnej zgodnej z OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="462"/>
        <source>An OpenPGP Smart Card is a physical device that securely stores your private cryptographic keys and can be used for digital signing, encryption, and authentication. Popular examples include YubiKey, Nitrokey, and other GnuPG-compatible tokens.</source>
        <translation>Karta OpenPGP to fizyczne urządzenie, które bezpiecznie przechowuje Twoje prywatne klucze kryptograficzne i może służyć do podpisu cyfrowego, szyfrowania i uwierzytelniania. Popularne przykłady to YubiKey, Nitrokey i inne tokeny zgodne z GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="468"/>
        <source>Make sure your card is inserted and properly recognized by the system. You can also try reconnecting the card or restarting the application.</source>
        <translation>Upewnij się, że karta jest włożona i poprawnie rozpoznawana przez system. Możesz też spróbować podłączyć kartę ponownie lub ponownie uruchomić aplikację.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="483"/>
        <source>Note: Smart card support of GpgFrontend requires GnuPG version 2.3.0 or later.</source>
        <translation>Uwaga: obsługa kart inteligentnych w GpgFrontend wymaga GnuPG w wersji 2.3.0 lub nowszej.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="626"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="641"/>
        <source>Modify Card Attribute</source>
        <translation>Zmień atrybut karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="660"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="674"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="661"/>
        <source>Attribute operation completed successfully.</source>
        <translation>Operacja na atrybucie zakończona pomyślnie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="228"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="675"/>
        <source>PIN operation completed successfully.</source>
        <translation>Operacja na PIN-ie zakończona pomyślnie.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::StatusIndicatorInfo</name>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="57"/>
        <source>Click to manage profiles.</source>
        <translation>Kliknij, aby zarządzać profilami.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="62"/>
        <source>Click to see the full status.</source>
        <translation>Kliknij, aby zobaczyć pełny status.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="74"/>
        <source>Profile</source>
        <translation>Profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="80"/>
        <source>%1 (temporary)</source>
        <translation>%1 (tymczasowy)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="86"/>
        <source>Opened from a file, and not kept on this computer. Closing asks whether to save the changes back into it.</source>
        <translation>Otwarty z pliku, a nie przechowywany na tym komputerze. Przy zamykaniu pojawi się pytanie, czy zapisać zmiany z powrotem do niego.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="90"/>
        <source>This window&apos;s profile — its own settings, keys and saved state</source>
        <translation>Profil tego okna — własne ustawienia, klucze i zapisany stan</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="99"/>
        <source>Kept in %1.</source>
        <translation>Przechowywany w %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="107"/>
        <source>This profile&apos;s own key is held in memory only.</source>
        <translation>Własny klucz tego profilu jest przechowywany wyłącznie w pamięci.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="131"/>
        <source>Current OpenPGP backend and version</source>
        <translation>Bieżący silnik OpenPGP i jego wersja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="135"/>
        <source>Key database: %1</source>
        <translation>Baza kluczy: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="152"/>
        <source>Portable Mode</source>
        <translation>Tryb przenośny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="154"/>
        <source>Installed Mode</source>
        <translation>Tryb zainstalowany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="159"/>
        <source>Running from the folder it was unpacked into, taking its settings and keys along with it.</source>
        <translation>Działa z folderu, do którego został rozpakowany, zabierając swoje ustawienia i klucze ze sobą.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="163"/>
        <source>Installed on this computer, with its settings and keys kept in this user&apos;s data folder.</source>
        <translation>Zainstalowany na tym komputerze, z ustawieniami i kluczami przechowywanymi w folderze danych tego użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="167"/>
        <source>This profile keeps its own keys, separate from the rest of the computer.</source>
        <translation>Ten profil przechowuje własne klucze, oddzielnie od reszty komputera.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::StatusTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="491"/>
        <source>GnuPG Home:</source>
        <translation>Katalog domowy GnuPG:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="492"/>
        <source>Unusable</source>
        <translation>nienadający się do użytku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="500"/>
        <source>Portable Mode</source>
        <translation>Tryb przenośny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="500"/>
        <source>Installed Mode</source>
        <translation>Tryb zainstalowany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="506"/>
        <source>Application</source>
        <translation>Aplikacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="514"/>
        <source>Profile ID:</source>
        <translation>Identyfikator profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="551"/>
        <source>Profile Key Rotation:</source>
        <translation>Rotacja klucza profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="552"/>
        <source>On a schedule</source>
        <translation>Według harmonogramu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="553"/>
        <source>New saved data uses the current period&apos;s key, and the keys that open what earlier periods wrote are kept alongside it.</source>
        <translation>Nowo zapisywane dane używają klucza bieżącego okresu, a klucze otwierające to, co zapisano we wcześniejszych okresach, są przechowywane obok niego.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="563"/>
        <source>Profile Format Version:</source>
        <translation>Wersja formatu profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="573"/>
        <source>Profile File:</source>
        <translation>Plik profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="582"/>
        <source>Imported From:</source>
        <translation>Zaimportowano z:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="615"/>
        <source>Folders</source>
        <translation>Foldery</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="621"/>
        <source>Available</source>
        <translation>Dostępne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="626"/>
        <source>Active</source>
        <translation>Aktywny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="626"/>
        <source>Disabled</source>
        <translation>Wyłączony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="635"/>
        <source>Default Pinentry Program</source>
        <translation>Domyślny program Pinentry</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="640"/>
        <source>OpenPGP Engines</source>
        <translation>Silniki OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="661"/>
        <source>Needs Attention</source>
        <translation>Wymaga uwagi</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="662"/>
        <source>At a Glance</source>
        <translation>W skrócie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="679"/>
        <source>More details</source>
        <translation>Więcej szczegółów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="679"/>
        <source>Details</source>
        <translation>Szczegóły</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="684"/>
        <source>Copy Status Information</source>
        <translation>Skopiuj informacje o statusie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="692"/>
        <source>These values reflect the current startup environment and may help when reporting issues.</source>
        <translation>Te wartości odzwierciedlają bieżące środowisko startowe i mogą pomóc przy zgłaszaniu problemów.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="476"/>
        <source>Secure Level:</source>
        <translation>Poziom bezpieczeństwa:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="473"/>
        <source>Application Key Protection:</source>
        <translation>Ochrona klucza aplikacji:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="480"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="502"/>
        <source>System Credential Store:</source>
        <translation>Systemowy magazyn poświadczeń:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="481"/>
        <source>Unavailable</source>
        <translation>Niedostępny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="499"/>
        <source>Running Mode:</source>
        <translation>Tryb działania:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="625"/>
        <source>GnuPG Offline Mode:</source>
        <translation>Tryb offline GnuPG:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="633"/>
        <source>Pinentry Program Path:</source>
        <translation>Ścieżka programu Pinentry:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="592"/>
        <source>Profile Folder:</source>
        <translation>Folder profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="599"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="601"/>
        <source>Workspace:</source>
        <translation>Obszar roboczy:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="599"/>
        <source>None</source>
        <translation>Brak</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="611"/>
        <source>Profiles Folder:</source>
        <translation>Folder profili:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="586"/>
        <source>Profile</source>
        <translation>Profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="525"/>
        <source>Session Storage:</source>
        <translation>Magazyn sesji:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="536"/>
        <source>Profile Key:</source>
        <translation>Klucz profilu:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="537"/>
        <source>Held in memory only</source>
        <translation>Przechowywany tylko w pamięci</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="538"/>
        <source>The key that protects this profile&apos;s own saved data is never written here. Your OpenPGP keys are a separate thing and do live in the session storage above, because GnuPG needs real files for them.</source>
        <translation>Klucz chroniący własne zapisane dane tego profilu nigdy nie jest tutaj zapisywany. Twoje klucze OpenPGP to co innego i faktycznie żyją w magazynie sesji powyżej, ponieważ GnuPG potrzebuje dla nich prawdziwych plików.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SubkeyGenerateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="150"/>
        <source>Tipps: if the key pair has a passphrase, the subkey&apos;s passphrase must be equal to it.</source>
        <translation>Wskazówka: jeśli para kluczy ma hasło, hasło podklucza musi być takie samo.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="166"/>
        <source>Generate New Subkey</source>
        <translation>Wygeneruj nowy podklucz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="126"/>
        <source>Authentication</source>
        <translation>Uwierzytelnianie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="120"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="121"/>
        <source>Key Length</source>
        <translation>Długość klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="122"/>
        <source>Expire Date</source>
        <translation>Data wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="123"/>
        <source>Usage</source>
        <translation>Zastosowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="124"/>
        <source>Encrypt</source>
        <translation>Szyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="125"/>
        <source>Sign</source>
        <translation>Podpisywanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="127"/>
        <source>Non Expired</source>
        <translation>Bez wygaśnięcia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="128"/>
        <source>No Passphrase</source>
        <translation>Bez hasła</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="129"/>
        <source>Second Algorithm</source>
        <translation>Drugi algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="130"/>
        <source>Second Key Length</source>
        <translation>Druga długość klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="161"/>
        <source>Note: post-quantum (PQC) subkey algorithms are unavailable here because the primary key uses the v4 key format. Generate a v6 key to use PQC algorithms.</source>
        <translation>Uwaga: pokwantowe (PQC) algorytmy podkluczy są tutaj niedostępne, ponieważ klucz główny używa formatu v4. Aby korzystać z algorytmów PQC, wygeneruj klucz v6.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="474"/>
        <source>Please give a valid subkey algorithm.</source>
        <translation>Podaj prawidłowy algorytm podklucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="484"/>
        <source>Please give a valid second algorithm.</source>
        <translation>Podaj prawidłowy drugi algorytm.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="494"/>
        <source>Time to subkey expiration must not be less than 120 seconds.</source>
        <translation>Czas do wygaśnięcia podklucza nie może być mniejszy niż 120 sekund.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="511"/>
        <source>Generating</source>
        <translation>Generowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="524"/>
        <source>Error</source>
        <translation>Błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="525"/>
        <source>Unknown error occurred</source>
        <translation>Wystąpił nieznany błąd</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="553"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="554"/>
        <source>Subkey generation completed successfully.</source>
        <translation>Generowanie podklucza zakończone pomyślnie.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TextEdit</name>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="208"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="646"/>
        <source>Warning</source>
        <translation>Ostrzeżenie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="209"/>
        <source>Cannot read file %1:
%2.</source>
        <translation>Nie można odczytać pliku %1:
%2.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="118"/>
        <source>Open file</source>
        <translation>Otwórz plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="95"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="103"/>
        <source>Open File</source>
        <translation>Otwórz plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="605"/>
        <source>File Open Error</source>
        <translation>Błąd otwierania pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="606"/>
        <source>The file &quot;%1&quot; could not be opened.</source>
        <translation>Nie udało się otworzyć pliku „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="137"/>
        <source>Unknown Tab Type</source>
        <translation>Nieznany typ karty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="138"/>
        <source>The current tab has an unknown type. Cannot save.</source>
        <translation>Bieżąca karta ma nieznany typ. Nie można zapisać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="157"/>
        <source>Unsupported Operation</source>
        <translation>Nieobsługiwana operacja</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="158"/>
        <source>The save file operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Operacja zapisu pliku dla typu karty „%1” nie jest obsługiwana.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="249"/>
        <source>Save file</source>
        <translation>Zapisz plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="317"/>
        <source>Unsaved document</source>
        <translation>Niezapisany dokument</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="318"/>
        <source>The document &quot;%1&quot; has been modified. Do you want to save your changes?</source>
        <translation>Dokument „%1” został zmodyfikowany. Czy chcesz zapisać zmiany?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="321"/>
        <source>Note:</source>
        <translation>Uwaga:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="322"/>
        <source>If you don&apos;t save these files, all changes are lost.</source>
        <translation>Jeśli nie zapiszesz tych plików, wszystkie zmiany przepadną.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="646"/>
        <source>No document to print</source>
        <translation>Brak dokumentu do wydrukowania</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TextEditTabWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="336"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="359"/>
        <source>File Open Error</source>
        <translation>Błąd otwierania pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="282"/>
        <source>The file &quot;%1&quot; could not be opened.</source>
        <translation>Nie udało się otworzyć pliku „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="155"/>
        <source>Close</source>
        <translation>Zamknij</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="156"/>
        <source>Copy Path</source>
        <translation>Skopiuj ścieżkę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="157"/>
        <source>Reveal in File Browser</source>
        <translation>Pokaż w menedżerze plików</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="262"/>
        <source>The file does not exist.</source>
        <translation>Plik nie istnieje.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="271"/>
        <source>The file &quot;%1&quot; is larger than 4 MB and will not be opened in the text editor.</source>
        <translation>Plik „%1” jest większy niż 4 MB i nie zostanie otwarty w edytorze tekstu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="290"/>
        <source>The file &quot;%1&quot; appears to be a binary file and will not be opened.</source>
        <translation>Plik „%1” wygląda na plik binarny i nie zostanie otwarty.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="337"/>
        <source>The file &quot;%1&quot; does not exist.</source>
        <translation>Plik „%1” nie istnieje.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="537"/>
        <source>Select Default Path</source>
        <translation>Wybierz domyślną ścieżkę</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="602"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1097"/>
        <source>Workspace</source>
        <translation>Obszar roboczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1069"/>
        <source>Directory Permission Denied</source>
        <translation>Brak uprawnień do katalogu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1070"/>
        <source>You do not have permission to access the directory &quot;%1&quot;.</source>
        <translation>Nie masz uprawnień do dostępu do katalogu „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1092"/>
        <source>Root</source>
        <translation>Katalog główny</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TranslatorsTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="399"/>
        <source>Thanks to all translators</source>
        <translation>Podziękowania dla wszystkich tłumaczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="409"/>
        <source>Translator information is not available.</source>
        <translation>Informacje o tłumaczach są niedostępne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="413"/>
        <source>If you want to help improve localization, please read the &lt;a href=&apos;https://gpgfrontend.bktus.com/appendix/translate-interface/&apos;&gt;translation guide&lt;/a&gt;.!</source>
        <translation>Jeśli chcesz pomóc ulepszać lokalizację, przeczytaj &lt;a href=&apos;https://gpgfrontend.bktus.com/appendix/translate-interface/&apos;&gt;przewodnik tłumaczenia&lt;/a&gt;.!</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::WaitingDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/WaitingDialog.cpp" line="46"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/WaitingDialog.cpp" line="84"/>
        <source>Cancelling...</source>
        <translation>Anulowanie...</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::Wizard</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="225"/>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="232"/>
        <source>Welcome to GpgFrontend</source>
        <translation>Witamy w GpgFrontend</translation>
    </message>
</context>
<context>
    <name>GpgKey</name>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="107"/>
        <source>Unknown</source>
        <translation>Nieznane</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="109"/>
        <source>Undefined</source>
        <translation>Nieokreślone</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="111"/>
        <source>Never</source>
        <translation>Nigdy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="113"/>
        <source>Marginal</source>
        <translation>Marginalne</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="115"/>
        <source>Full</source>
        <translation>Pełne</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="103"/>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="117"/>
        <source>Ultimate</source>
        <translation>Ostateczne</translation>
    </message>
</context>
<context>
    <name>ProfileAccessor</name>
    <message>
        <location filename="../../../../src/core/profile/ProfileAccessor.cpp" line="123"/>
        <source>an ordinary folder on this disk</source>
        <translation>zwykły folder na tym dysku</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProtectedFsProfileAccessor.cpp" line="427"/>
        <source>memory only; not written to your disk in the normal course of things</source>
        <translation>tylko w pamięci; nie jest zapisywany na dysk w normalnych okolicznościach</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProtectedFsProfileAccessor.cpp" line="433"/>
        <source>an encrypted folder this session alone can read</source>
        <translation>zaszyfrowany folder, który może odczytać tylko ta sesja</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProtectedFsProfileAccessor.cpp" line="437"/>
        <source>an ordinary temporary folder on this disk</source>
        <translation>zwykły folder tymczasowy na tym dysku</translation>
    </message>
</context>
<context>
    <name>ProfileSecureKeyManager</name>
    <message>
        <location filename="../../../../src/core/profile/ProfileSecureKeyManager.cpp" line="443"/>
        <source>this session&apos;s memory, and nowhere on this machine</source>
        <translation>pamięć tej sesji i nigdzie indziej na tej maszynie</translation>
    </message>
</context>
<context>
    <name>QCoreApplication</name>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="153"/>
        <source>File Hash Information</source>
        <translation>Informacje o skrócie pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="127"/>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="170"/>
        <source>Filename</source>
        <translation>Nazwa pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="155"/>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="170"/>
        <source>: </source>
        <translation>: </translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="128"/>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="130"/>
        <source>File Size</source>
        <translation>Rozmiar pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="168"/>
        <source>Error: cannot read target file</source>
        <translation>Błąd: nie można odczytać pliku docelowego</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="202"/>
        <source>target path doesn&apos;t exists</source>
        <translation>ścieżka docelowa nie istnieje</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="208"/>
        <source>do NOT have permission to write path</source>
        <translation>brak uprawnień do zapisu w ścieżce</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="214"/>
        <source>do NOT have permission to read/write file</source>
        <translation>brak uprawnień do odczytu/zapisu pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="217"/>
        <source>Success</source>
        <translation>Sukces</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1705"/>
        <source>Sync All Public Key</source>
        <translation>Synchronizuj wszystkie klucze publiczne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1706"/>
        <source>You have not checked any public keys that you want to synchronize, do you want to synchronize all local public keys from the key server?</source>
        <translation>Nie zaznaczono żadnych kluczy publicznych do synchronizacji. Czy zsynchronizować wszystkie lokalne klucze publiczne z serwera kluczy?</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="540"/>
        <source>Basic Path Initiation Failed</source>
        <translation>Inicjowanie podstawowych ścieżek nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="568"/>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="638"/>
        <source>No Supported OpenPGP Engine Detected</source>
        <translation>Nie wykryto obsługiwanego silnika OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="610"/>
        <source>No valid Key Database</source>
        <translation>Brak prawidłowej bazy kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="670"/>
        <source>GpgME Default Context Initiation Failed</source>
        <translation>Inicjowanie domyślnego kontekstu GpgME nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="683"/>
        <source>Gpg Default Key Database Initiation Failed</source>
        <translation>Inicjowanie domyślnej bazy kluczy Gpg nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="102"/>
        <source>Starting GpgFrontend</source>
        <translation>Uruchamianie GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="108"/>
        <source>Loading essential information</source>
        <translation>Wczytywanie istotnych informacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="114"/>
        <source>GpgFrontend is checking your OpenPGP environment and preparing the default engine. This may take a few seconds.</source>
        <translation>GpgFrontend sprawdza Twoje środowisko OpenPGP i przygotowuje domyślny silnik. Może to potrwać kilka sekund.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="119"/>
        <source>Please keep this window open while the initialization is running.</source>
        <translation>Nie zamykaj tego okna, dopóki trwa inicjalizacja.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="132"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../../../../src/GpgFrontend.cpp" line="107"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="249"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="273"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="230"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="619"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="765"/>
        <source>Cannot Open Profile</source>
        <translation>Nie można otworzyć profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="366"/>
        <source>Save Key Failed</source>
        <translation>Zapis klucza nie powiódł się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="401"/>
        <source>Secure Key Generation Failed</source>
        <translation>Generowanie klucza bezpieczeństwa nie powiodło się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="367"/>
        <source>The application secure key could not be saved: %1</source>
        <translation>Nie udało się zapisać klucza bezpieczeństwa aplikacji: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="63"/>
        <source>Resetting lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Resetowanie pozwala aplikacji wystartować, ale wszystko, co wcześniej zaszyfrowała, stanie się trwale nieczytelne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="70"/>
        <source>You can unlock the keychain and start the application again instead. That costs nothing and leaves the key intact.</source>
        <translation>Zamiast tego możesz odblokować brelok kluczy i uruchomić aplikację ponownie. To nic nie kosztuje i pozostawia klucz nienaruszonym.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="162"/>
        <source>It is open in process %1 on %2.</source>
        <translation>Jest otwarty w procesie %1 na %2.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="164"/>
        <source>this computer</source>
        <translation>tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="166"/>
        <source>Another process has it open.</source>
        <translation>Inny proces ma go otwartego.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="169"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="255"/>
        <source>Profile Is Already Open</source>
        <translation>Profil jest już otwarty</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="170"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="256"/>
        <source>This profile is already open in another window.</source>
        <translation>Ten profil jest już otwarty w innym oknie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="172"/>
        <source>Opening it twice would corrupt its stored data.</source>
        <translation>Otwarcie go dwukrotnie uszkodziłoby jego zapisane dane.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="173"/>
        <source>Profile: %1</source>
        <translation>Profil: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="194"/>
        <source>Force Unlock</source>
        <translation>Wymuś odblokowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="178"/>
        <source>Only do this if you are certain no other GpgFrontend window has this profile open.</source>
        <translation>Zrób to tylko wtedy, gdy masz pewność, że żadne inne okno GpgFrontend nie ma otwartego tego profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="181"/>
        <source>If one does, both copies will corrupt the profile&apos;s stored data.</source>
        <translation>Jeśli jakieś ma, obie kopie uszkodzą zapisane dane profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="263"/>
        <source>Cannot Lock Profile</source>
        <translation>Nie można zablokować profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="264"/>
        <source>The lock file at %1 could not be created.</source>
        <translation>Nie udało się utworzyć pliku blokady w %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="281"/>
        <source>This profile is set to open only in storage that is not left readable on this disk, and none is available here.</source>
        <translation>Ten profil jest ustawiony tak, aby otwierał się wyłącznie w pamięci, która nie pozostaje odczytywalna na tym dysku, a taka nie jest tu dostępna.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="286"/>
        <source>Change &apos;Profile package storage&apos; in Settings to allow an ordinary temporary folder.</source>
        <translation>Zmień „Miejsce przechowywania pakietu profilu” w Ustawieniach, aby zezwolić na zwykły folder tymczasowy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="290"/>
        <source>Cannot Open Profile Safely</source>
        <translation>Nie można bezpiecznie otworzyć profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="297"/>
        <source>Not Enough Room</source>
        <translation>Za mało miejsca</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="298"/>
        <source>There was not enough room to unpack this profile.</source>
        <translation>Nie było wystarczająco miejsca, aby rozpakować ten profil.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="301"/>
        <source>Storage: %1</source>
        <translation>Pamięć: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="303"/>
        <source>The file itself is fine — free some space and try again.</source>
        <translation>Sam plik jest w porządku — zwolnij trochę miejsca i spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="310"/>
        <source>This Profile File Is Too Large</source>
        <translation>Ten plik profilu jest za duży</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="311"/>
        <source>Opening a profile file needs to hold all of it in memory at once, and this one is bigger than this version can manage.</source>
        <translation>Otwarcie pliku profilu wymaga trzymania go w całości w pamięci naraz, a ten jest większy, niż ta wersja jest w stanie obsłużyć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="317"/>
        <source>Size / limit: %1</source>
        <translation>Rozmiar / limit: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="319"/>
        <source>Nothing has been unpacked. This version cannot create a file this large either, so it was made somewhere else or has been damaged.</source>
        <translation>Nic nie zostało rozpakowane. Ta wersja nie potrafi też utworzyć tak dużego pliku, więc powstał gdzie indziej albo został uszkodzony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="326"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="129"/>
        <source>This File Has Been Altered</source>
        <translation>Ten plik został zmieniony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="338"/>
        <source>Profile Cannot Be Opened</source>
        <translation>Nie można otworzyć profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="343"/>
        <source>Continuing would damage it. Please use %1 or later, or start this version with a different profile.</source>
        <translation>Kontynuacja uszkodziłaby go. Użyj %1 lub nowszej albo uruchom tę wersję z innym profilem.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="353"/>
        <source>Profile Upgrade Failed</source>
        <translation>Aktualizacja profilu nie powiodła się</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="354"/>
        <source>Upgrading this profile stopped at step &apos;%1&apos;.</source>
        <translation>Aktualizacja tego profilu zatrzymała się na kroku „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="357"/>
        <source>The profile is intact. Please report this.</source>
        <translation>Profil jest nienaruszony. Zgłoś to, proszę.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="370"/>
        <source>Anything saved now would be unreadable after a restart, so the application will not continue. Please check your storage and permissions.</source>
        <translation>Wszystko zapisane teraz byłoby nieczytelne po ponownym uruchomieniu, więc aplikacja nie będzie kontynuować. Sprawdź pamięć i uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="380"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="389"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="409"/>
        <source>App Secure Key Error</source>
        <translation>Błąd klucza bezpieczeństwa aplikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="381"/>
        <source>Failed to decrypt the application secure key. The key file may be corrupted.</source>
        <translation>Nie udało się odszyfrować klucza bezpieczeństwa aplikacji. Plik klucza może być uszkodzony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="383"/>
        <source>Please clear the secure key and try again.</source>
        <translation>Wyczyść klucz bezpieczeństwa i spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="390"/>
        <source>Failed to read the application secure key from disk at: %1</source>
        <translation>Nie udało się odczytać klucza bezpieczeństwa aplikacji z dysku pod adresem: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="394"/>
        <source>Please ensure the key file exists and is accessible, or try re-initializing the secure key.</source>
        <translation>Upewnij się, że plik klucza istnieje i jest dostępny, albo spróbuj ponownie zainicjować klucz bezpieczeństwa.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="402"/>
        <source>Failed to generate an application secure key.</source>
        <translation>Nie udało się wygenerować klucza bezpieczeństwa aplikacji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="403"/>
        <source>Please check your system&apos;s cryptography support.</source>
        <translation>Sprawdź obsługę kryptografii w swoim systemie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="229"/>
        <source>Reset Secure Key</source>
        <translation>Resetuj klucz bezpieczeństwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="429"/>
        <source>System Keychain Unavailable</source>
        <translation>Systemowy brelok kluczy niedostępny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="430"/>
        <source>The application key could not be protected using the system keychain, so it remains stored unprotected.</source>
        <translation>Nie udało się chronić klucza aplikacji przy użyciu systemowego breloka kluczy, więc pozostaje przechowywany bez ochrony.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="433"/>
        <source>This setting has been turned off. You can turn it on again once a keychain is available.</source>
        <translation>To ustawienie zostało wyłączone. Możesz je ponownie włączyć, gdy brelok kluczy będzie dostępny.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="410"/>
        <source>The application secure key at %1 could not be read or rewritten.</source>
        <translation>Nie udało się odczytać ani przepisać klucza bezpieczeństwa aplikacji w %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="266"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="413"/>
        <source>Please check your storage and permissions.</source>
        <translation>Sprawdź pamięć i uprawnienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="205"/>
        <source>Secure Key Unavailable</source>
        <translation>Klucz bezpieczeństwa niedostępny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="206"/>
        <source>The application key is protected by a secret kept in the system keychain, and that secret could not be read.</source>
        <translation>Klucz aplikacji jest chroniony sekretem przechowywanym w systemowym breloku kluczy, a tego sekretu nie udało się odczytać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="210"/>
        <source>This usually means the keychain is locked, was reset, or this profile was copied from another computer or user account.</source>
        <translation>Zwykle oznacza to, że brelok kluczy jest zablokowany, został zresetowany albo ten profil został skopiowany z innego komputera lub konta użytkownika.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="214"/>
        <source>You can unlock the keychain and start the application again. Resetting the key instead lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Możesz odblokować brelok kluczy i uruchomić aplikację ponownie. Resetowanie klucza pozwala aplikacji wystartować, ale wszystko, co wcześniej zaszyfrowała, stanie się trwale nieczytelne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="192"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="228"/>
        <source>Quit</source>
        <translation>Zakończ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="85"/>
        <source>Reset the secure key?</source>
        <translation>Zresetować klucz bezpieczeństwa?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="58"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="74"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="82"/>
        <source>Reset to Default</source>
        <translation>Przywróć domyślne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="60"/>
        <source>Resetting removes the PIN and lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Resetowanie usuwa PIN i pozwala aplikacji wystartować, ale wszystko, co wcześniej zaszyfrowała, stanie się trwale nieczytelne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="68"/>
        <source>Only do this if you cannot recall the PIN. There is no other way to recover the key.</source>
        <translation>Zrób to tylko wtedy, gdy nie pamiętasz PIN-u. Nie ma innego sposobu na odzyskanie klucza.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="73"/>
        <source>Go Back</source>
        <translation>Wróć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="83"/>
        <source>Everything the application has encrypted with the current key will be permanently unreadable.</source>
        <translation>Wszystko, co aplikacja zaszyfrowała bieżącym kluczem, stanie się trwale nieczytelne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="333"/>
        <source>a newer version</source>
        <translation>nowszej wersji GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="337"/>
        <source>Profile Is Too New</source>
        <translation>Profil jest zbyt nowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="339"/>
        <source>This application data was last used by %1, which stores it in a format this version does not understand.</source>
        <translation>Z tych danych aplikacji ostatnio korzystała wersja %1, która zapisuje je w formacie nieznajomym tej wersji.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="346"/>
        <source>Data folder: %1</source>
        <translation>Folder danych: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="450"/>
        <source>Application Key Not Protected</source>
        <translation>Klucz aplikacji nie jest chroniony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="451"/>
        <source>A PIN is configured, but the application key on disk is not encrypted.</source>
        <translation>PIN jest skonfigurowany, ale klucz aplikacji na dysku nie jest zaszyfrowany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="454"/>
        <source>This can happen if a previous change was interrupted. The PIN setting has been turned off and the key is left as it is.</source>
        <translation>Może się to zdarzyć, jeśli wcześniejsza zmiana została przerwana. Ustawienie PIN zostało wyłączone, a klucz pozostawiono tak, jak jest.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="128"/>
        <source>That PIN did not unlock the application key. Please try again.</source>
        <translation>Ten PIN nie odblokował klucza aplikacji. Spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="131"/>
        <source>If you have forgotten your PIN, the application key and everything encrypted with it cannot be recovered.</source>
        <translation>Jeśli nie pamiętasz PIN-u, klucza aplikacji i wszystkiego, co nim zaszyfrowano, nie będzie można odzyskać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="70"/>
        <source>Key Database Path Too Long</source>
        <translation>Ścieżka bazy kluczy za długa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="71"/>
        <source>This folder&apos;s path is too long for GnuPG&apos;s agent socket, and no short enough link to it could be made, so GnuPG could not start against it. Choose a folder with a shorter path.</source>
        <translation>Ścieżka tego folderu jest za długa dla gniazda agenta GnuPG i nie udało się utworzyć wystarczająco krótkiego dowiązania do niego, więc GnuPG nie mógł z nim wystartować. Wybierz folder o krótszej ścieżce.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="89"/>
        <source>Active</source>
        <translation>Aktywna</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="90"/>
        <source>Inactive</source>
        <translation>Nieaktywna</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="102"/>
        <source>GpgFrontend could not open this key database at the last start.</source>
        <translation>GpgFrontend nie mógł otworzyć tej bazy kluczy podczas ostatniego uruchomienia.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="115"/>
        <source>Channel</source>
        <translation>Kanał</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="115"/>
        <source>Name</source>
        <translation>Nazwa</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="116"/>
        <source>Engine</source>
        <translation>Silnik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="116"/>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="65"/>
        <source>%1  ·  %2% used  ·  %3</source>
        <translation>%1  ·  zajęte: %2%  ·  %3</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="71"/>
        <source>%1  ·  %2</source>
        <translation>%1  ·  %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="53"/>
        <source>%1 item(s)</source>
        <translation>%1 elementów</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="98"/>
        <source>Open Link</source>
        <translation>Otwórz link</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="99"/>
        <source>Could not open a web browser on this system.

The link has been copied to your clipboard:
%1</source>
        <translation>Nie udało się otworzyć przeglądarki internetowej w tym systemie.

Link został skopiowany do schowka:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="127"/>
        <source>Star GpgFrontend on GitHub</source>
        <translation>Daj gwiazdkę GpgFrontend na GitHubie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="134"/>
        <source>GpgFrontend is free and open source. A star helps more people discover it and keeps the project moving forward.</source>
        <translation>GpgFrontend jest darmowy i open source. Gwiazdka pomaga większej liczbie osób go odkryć i utrzymuje projekt w rozwoju.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="76"/>
        <source>Very weak</source>
        <translation>Bardzo słabe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="79"/>
        <source>Weak</source>
        <translation>Słabe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="82"/>
        <source>Fair</source>
        <translation>Przeciętne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="85"/>
        <source>Good</source>
        <translation>Dobre</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="87"/>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="43"/>
        <source>Strong</source>
        <translation>Silne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="99"/>
        <source>Fully Valid</source>
        <translation>W pełni prawidłowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="101"/>
        <source>Valid (with Issues)</source>
        <translation>Prawidłowy (z problemami)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="103"/>
        <source>Valid (Not Fully Trusted)</source>
        <translation>Prawidłowy (bez pełnego zaufania)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="105"/>
        <source>Invalid</source>
        <translation>Nieprawidłowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="107"/>
        <source>Key Missing</source>
        <translation>Brak klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="109"/>
        <source>Key Revoked</source>
        <translation>Klucz unieważniony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="111"/>
        <source>Signature Expired</source>
        <translation>Podpis wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="113"/>
        <source>Key Expired</source>
        <translation>Klucz wygasł</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="115"/>
        <source>Verification Error</source>
        <translation>Błąd weryfikacji</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="117"/>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="47"/>
        <source>Unknown</source>
        <translation>Nieznany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="123"/>
        <source>Signer</source>
        <translation>Podpisujący</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="125"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="146"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="193"/>
        <source>Fingerprint</source>
        <translation>Odcisk klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="127"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="148"/>
        <source>Key ID</source>
        <translation>Identyfikator klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="133"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="151"/>
        <source>Algorithm</source>
        <translation>Algorytm</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="136"/>
        <source>Signed</source>
        <translation>Podpisano</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="144"/>
        <source>Recipient</source>
        <translation>Odbiorca</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="150"/>
        <source>Primary Key Algorithm</source>
        <translation>Algorytm klucza głównego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="163"/>
        <source>Input Material Hash</source>
        <translation>Skrót materiału wejściowego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="165"/>
        <source>SHA-256</source>
        <translation>SHA-256</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="182"/>
        <source>Signature Created</source>
        <translation>Utworzono podpis</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="185"/>
        <source>Mode</source>
        <translation>Tryb</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="191"/>
        <source>Invalid Signer</source>
        <translation>Nieprawidłowy podpisujący</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="194"/>
        <source>Reason</source>
        <translation>Powód</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="201"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="229"/>
        <source>Decrypt</source>
        <translation>Odszyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="204"/>
        <source>Message Metadata</source>
        <translation>Metadane wiadomości</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="140"/>
        <source>Open a Profile File</source>
        <translation>Otwórz plik profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="143"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="222"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="637"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="649"/>
        <source>Open</source>
        <translation>Otwórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="154"/>
        <source>Protect This Profile File</source>
        <translation>Chroń ten plik profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="157"/>
        <source>Save</source>
        <translation>Zapisz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="54"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="120"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="207"/>
        <source>File</source>
        <translation>Plik</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="57"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="123"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="117"/>
        <source>Folder</source>
        <translation>Folder</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="65"/>
        <source>Size</source>
        <translation>Rozmiar</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="66"/>
        <source>Modified</source>
        <translation>Zmodyfikowano</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="80"/>
        <source>Protection</source>
        <translation>Ochrona</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="81"/>
        <source>Not sealed</source>
        <translation>Bez zapieczętowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="82"/>
        <source>Anyone holding this file can read what is in it.</source>
        <translation>Każdy, kto posiada ten plik, może odczytać jego zawartość.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="89"/>
        <source>Created</source>
        <translation>Utworzono</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="96"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="191"/>
        <source>Written by</source>
        <translation>Program</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="98"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="192"/>
        <source>GpgFrontend %1</source>
        <translation>GpgFrontend %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="99"/>
        <source>GpgFrontend %1 (development build)</source>
        <translation>GpgFrontend %1 (wersja rozwojowa)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="109"/>
        <source>From the file&apos;s header (unverified)</source>
        <translation>Na podstawie nagłówka pliku (niezweryfikowane)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="133"/>
        <source>Free space</source>
        <translation>Wolne miejsce</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="142"/>
        <source>Enter the passphrase that opens this file.</source>
        <translation>Wprowadź hasło, które otwiera ten plik.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="156"/>
        <source>Choose the passphrase this file will be sealed with.</source>
        <translation>Wybierz hasło, którym plik zostanie zapieczętowany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="161"/>
        <source>A different passphrase replaces the old one, which will no longer open this file.</source>
        <translation>Inne hasło zastępuje stare, które nie będzie już otwierać tego pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="164"/>
        <source>If you lose this passphrase, this file can never be opened again.</source>
        <translation>Jeśli utracisz to hasło, tego pliku nigdy nie będzie można otworzyć.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="169"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="170"/>
        <source>Passphrase</source>
        <translation>Hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="171"/>
        <source>Repeat</source>
        <translation>Powtórz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="172"/>
        <source>Show passphrase</source>
        <translation>Pokaż hasło</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="173"/>
        <source>Strength</source>
        <translation>Siła</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="176"/>
        <source>The passphrase must be at least %1 characters.</source>
        <translation>Hasło musi mieć co najmniej %1 znaków.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="177"/>
        <source>The two entries do not match.</source>
        <translation>Wprowadzone hasła nie są zgodne.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="164"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="208"/>
        <source>Cipher</source>
        <translation>Szyfr</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="209"/>
        <source>MIME</source>
        <translation>MIME</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="210"/>
        <source>Yes</source>
        <translation>Tak</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="211"/>
        <source>No</source>
        <translation>Nie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="212"/>
        <source>Integrity</source>
        <translation>Integralność</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="214"/>
        <source>Protected</source>
        <translation>Chroniona</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="215"/>
        <source>Not Protected (unsafe)</source>
        <translation>Niechroniona (niebezpieczna)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="219"/>
        <source>Encrypt</source>
        <translation>Szyfrowanie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="222"/>
        <source>Encryption Recipient</source>
        <translation>Odbiorca szyfrowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="232"/>
        <source>Decryption Recipient</source>
        <translation>Odbiorca odszyfrowania</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="196"/>
        <source>ECC</source>
        <translation>ECC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="203"/>
        <source>Post-Quantum</source>
        <translation>Pokwantowe</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProfileSecureKeyManager.cpp" line="540"/>
        <source>The secure key could not be encrypted, so it was not saved to disk.</source>
        <translation>Nie udało się zaszyfrować klucza bezpieczeństwa, więc nie został zapisany na dysk.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="39"/>
        <source>Standard</source>
        <translation>Standardowe</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="41"/>
        <source>Enhanced</source>
        <translation>Wzmocnione</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="45"/>
        <source>Maximum</source>
        <translation>Maksymalne</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="54"/>
        <source>System keychain</source>
        <translation>Systemowy brelok kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="56"/>
        <source>PIN at startup</source>
        <translation>PIN przy uruchomieniu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="60"/>
        <source>No extra protection</source>
        <translation>Brak dodatkowej ochrony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="98"/>
        <source>Home</source>
        <translation>Katalog domowy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="91"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="96"/>
        <source>The new window could not be started.</source>
        <translation>Nie udało się uruchomić nowego okna.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="105"/>
        <source>Name This Profile</source>
        <translation>Nazwij ten profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="106"/>
        <source>What should this profile be called on this computer?</source>
        <translation>Jak ten profil powinien się nazywać na tym komputerze?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="130"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="150"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="168"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="177"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="681"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="703"/>
        <source>Cannot Import Profile</source>
        <translation>Nie można zaimportować profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="151"/>
        <source>This profile was made by a newer version of GpgFrontend (%1).</source>
        <translation>Ten profil został utworzony w nowszej wersji GpgFrontend (%1).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="189"/>
        <source>Profile</source>
        <translation>Profil</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="195"/>
        <source>Workspace files</source>
        <translation>Pliki obszaru roboczego</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="197"/>
        <source>Included</source>
        <translation>Uwzględnione</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="198"/>
        <source>None in the file</source>
        <translation>Brak w pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="207"/>
        <source>Keys</source>
        <translation>Klucze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="208"/>
        <source>&quot;%1&quot; is unavailable</source>
        <translation>„%1” jest niedostępny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="209"/>
        <source>It pointed at keys kept outside the profile, which do not travel. Point it somewhere on this computer to use it.</source>
        <translation>Wskazuje na klucze przechowywane poza profilem, a takie klucze nie są przenoszone. Wskaż inne miejsce na tym komputerze, aby móc go używać.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="217"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="756"/>
        <source>&quot;%1&quot; is ready.</source>
        <translation>„%1” jest gotowy.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="216"/>
        <source>Profile Imported</source>
        <translation>Profil zaimportowany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="219"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="757"/>
        <source>Open it now? It opens in a new window.</source>
        <translation>Otworzyć go teraz? Otwiera się w nowym oknie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="223"/>
        <source>Not Now</source>
        <translation>Nie teraz</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="266"/>
        <source>GpgFrontend Profile File</source>
        <translation>Plik profilu GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="335"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="350"/>
        <source>Default</source>
        <translation>Domyślny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="337"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="352"/>
        <source>Portable</source>
        <translation>Przenośny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="339"/>
        <source>From a package</source>
        <translation>Z pakietu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="341"/>
        <source>Local</source>
        <translation>Lokalny</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="388"/>
        <source>This file is no longer there:</source>
        <translation>Tego pliku nie ma już tutaj:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="397"/>
        <source>This window is already using that profile.</source>
        <translation>To okno już używa tego profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="407"/>
        <source>There is no profile called &quot;%1&quot;.</source>
        <translation>Nie ma profilu o nazwie „%1”.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="426"/>
        <source>&quot;%1&quot; is open in another window (process %2 on %3).</source>
        <translation>„%1” jest otwarty w innym oknie (proces %2 na %3).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="431"/>
        <source>&quot;%1&quot; is open in another window.</source>
        <translation>„%1” jest otwarty w innym oknie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="463"/>
        <source>Save Changes?</source>
        <translation>Zapisać zmiany?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="464"/>
        <source>This profile was opened from a file. It is not kept on this computer, and the copy it is running from is about to be deleted.</source>
        <translation>Ten profil został otwarty z pliku. Nie jest przechowywany na tym komputerze, a kopia, na której działa, zaraz zostanie usunięta.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="469"/>
        <source>Anything you changed is lost unless it is written back into that file.</source>
        <translation>Wszystkie zmiany przepadną, jeśli nie zostaną zapisane z powrotem do tego pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="474"/>
        <source>Save Changes</source>
        <translation>Zapisz zmiany</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="476"/>
        <source>Discard</source>
        <translation>Odrzuć</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="477"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="652"/>
        <source>Cancel</source>
        <translation>Anuluj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="561"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="583"/>
        <source>Cannot Save Changes</source>
        <translation>Nie można zapisać zmian</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="562"/>
        <source>The application key is not available, so the profile could not be packed.</source>
        <translation>Klucz aplikacji jest niedostępny, więc nie udało się spakować profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="570"/>
        <source>Saving Profile</source>
        <translation>Zapisywanie profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="626"/>
        <source>&quot;%1&quot; is a profile file. Choose what to do with it.</source>
        <translation>„%1” jest plikiem profilu. Wybierz, co z nim zrobić.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="636"/>
        <source>What happens next</source>
        <translation>Co się stanie dalej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="638"/>
        <source>Work in the file itself</source>
        <translation>Pracuj bezpośrednio w pliku</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="639"/>
        <source>Nothing is kept on this computer. What you change is written back into the file.</source>
        <translation>Na tym komputerze nic nie jest przechowywane. Wprowadzone zmiany są zapisywane z powrotem do pliku.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="643"/>
        <source>Copy it into a profile kept here</source>
        <translation>Skopiuj do profilu przechowywanego na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="644"/>
        <source>The copy lives on this computer from then on, and the file is not used again.</source>
        <translation>Od tej pory kopia jest przechowywana na tym komputerze, a plik nie jest już używany.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="664"/>
        <source>Import Profile File</source>
        <translation>Importuj plik profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="704"/>
        <source>A temporary folder could not be made.</source>
        <translation>Nie udało się utworzyć folderu tymczasowego.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="711"/>
        <source>Reading Profile</source>
        <translation>Odczytywanie profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="735"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="745"/>
        <source>Cannot Create Profile</source>
        <translation>Nie można utworzyć profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="169"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="736"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="746"/>
        <source>The profile could not be created.</source>
        <translation>Nie udało się utworzyć profilu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="625"/>
        <source>Open Profile File</source>
        <translation>Otwórz plik profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="642"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="651"/>
        <source>Import</source>
        <translation>Importuj</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="755"/>
        <source>Profile Created</source>
        <translation>Profil utworzony</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="159"/>
        <source>That passphrase did not open this file. Try again.</source>
        <translation>To hasło nie otworzyło tego pliku. Spróbuj ponownie.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="46"/>
        <source>Settings</source>
        <translation>Ustawienia</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="50"/>
        <source>Saved state, key groups and categories</source>
        <translation>Zapisany stan, grupy kluczy i kategorie</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="58"/>
        <source>This profile&apos;s own key</source>
        <translation>Własny klucz tego profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="62"/>
        <source>Keys stored inside this profile</source>
        <translation>Klucze przechowywane wewnątrz tego profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="65"/>
        <source>My workspace files</source>
        <translation>Moje pliki przestrzeni roboczej</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="119"/>
        <source>Writes %1: keys, settings and workspace files, about %2 before compression.</source>
        <translation>Zapisuje %1: klucze, ustawienia i pliki obszaru roboczego, około %2 przed kompresją.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="121"/>
        <source>Writes %1: keys and settings, about %2 before compression.</source>
        <translation>Zapisuje %1: klucze i ustawienia, około %2 przed kompresją.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="144"/>
        <source>Total</source>
        <translation>Razem</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="153"/>
        <source>Never included: logs, modules, and keys kept elsewhere.</source>
        <translation>Nigdy nie są uwzględniane: dzienniki, moduły oraz klucze przechowywane w innych miejscach.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="155"/>
        <source>Keys kept outside this profile, such as the system GnuPG keyring, stay where they are.</source>
        <translation>Klucze przechowywane poza tym profilem, takie jak systemowa baza kluczy GnuPG, pozostają na swoim miejscu.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="165"/>
        <source>Key derivation</source>
        <translation>Wyprowadzanie klucza</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="166"/>
        <source>Argon2id, from your passphrase</source>
        <translation>Argon2id, na podstawie twojego hasła</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="167"/>
        <source>Keychain</source>
        <translation>Pęk kluczy</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="168"/>
        <source>Not used: the file has to open on another computer</source>
        <translation>Nieużywany: plik musi dać się otworzyć na innym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="101"/>
        <source>There may not be room for this where you are saving it. The file is compressed as it is written, so it may still fit.</source>
        <translation>W miejscu zapisu może nie być wystarczająco miejsca. Plik jest kompresowany w trakcie zapisu, więc wciąż może się zmieścić.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="45"/>
        <source>This profile&apos;s own default key database</source>
        <translation>Własna domyślna baza kluczy tego profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="46"/>
        <source>This computer&apos;s default key database</source>
        <translation>Domyślna baza kluczy tego komputera</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="48"/>
        <source>Kept inside your profile</source>
        <translation>Przechowywana w twoim profilu</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="50"/>
        <source>On this computer only</source>
        <translation>Tylko na tym komputerze</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="90"/>
        <source>Channel %1</source>
        <translation>Kanał %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/MetaListPanel.cpp" line="81"/>
        <source>These come from the file&apos;s unencrypted header, which anyone holding the file can change.</source>
        <translation>Te dane pochodzą z niezaszyfrowanego nagłówka pliku, który może zmienić każdy, kto ma ten plik.</translation>
    </message>
</context>
</TS>
