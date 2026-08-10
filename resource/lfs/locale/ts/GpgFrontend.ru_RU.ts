<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
<context>
    <name>GpgFrontend</name>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1051"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1053"/>
        <source>Expiring Soon</source>
        <translation>Скоро истекает</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1055"/>
        <source>Expired</source>
        <translation>Срок истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1057"/>
        <source>Revoked</source>
        <translation>Отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1059"/>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgDecryptResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="40"/>
        <source>Decrypt</source>
        <translation>Расшифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="43"/>
        <source>Decrypt Operation</source>
        <translation>Операция расшифровки</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="46"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="48"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="52"/>
        <source>Detail</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="57"/>
        <source>Unsupported Algo</source>
        <translation>Неподдерживаемый алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="67"/>
        <source>General State</source>
        <translation>Общее состояние</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="75"/>
        <source>File Name</source>
        <translation>Имя файла</translation>
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
        <translation>нет</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="79"/>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="82"/>
        <source>true</source>
        <translation>да</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="81"/>
        <source>Message Integrity Protection</source>
        <translation>Защита целостности сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="89"/>
        <source>Symmetric Encryption Algorithm</source>
        <translation>Алгоритм симметричного шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="96"/>
        <source>Recipient(s)</source>
        <translation>Получатель(и)</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="103"/>
        <source>Recipient</source>
        <translation>Получатель</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="118"/>
        <source>Decrypted successfully. The original filename is &quot;%1&quot;.</source>
        <translation>Расшифровка выполнена успешно. Исходное имя файла: &quot;%1&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="121"/>
        <source>The message has been decrypted successfully and is now readable.</source>
        <translation>Сообщение успешно расшифровано и теперь доступно для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="127"/>
        <source>Decrypted, but message integrity protection is absent. This may be a legacy message that cannot detect tampering.</source>
        <translation>Расшифровано, но защита целостности сообщения отсутствует. Возможно, это устаревшее сообщение, в котором невозможно обнаружить подмену.</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="136"/>
        <source>Decrypted, but %n recipient key(s) could not be found in your keyring.</source>
        <translation>
            <numerusform>Расшифровано, но в вашей связке ключей не найдено %n ключ(ей) получателя(ей).</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="139"/>
        <source>Decrypted with warnings. Please review the details.</source>
        <translation>Расшифровано с предупреждениями. Пожалуйста, ознакомьтесь с деталями.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="144"/>
        <source>Decryption failed: %1.</source>
        <translation>Расшифровка не удалась: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="171"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;unknown&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="179"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="183"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="184"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="190"/>
        <source>Public Key Algo</source>
        <translation>Алгоритм открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="192"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgEncryptResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="40"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="43"/>
        <source>Encrypt Operation</source>
        <translation>Операция шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="46"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="48"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="51"/>
        <source>Detail</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="63"/>
        <source>Invalid Recipients</source>
        <translation>Недопустимые получатели</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="70"/>
        <source>Recipients</source>
        <translation>Получатели</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="72"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="74"/>
        <source>Reason</source>
        <translation>Причина</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="92"/>
        <source>Encrypted with warnings. Some recipients could not be added. Please review the invalid recipient list.</source>
        <translation>Зашифровано с предупреждениями. Некоторые получатели не были добавлены. Пожалуйста, проверьте список недопустимых получателей.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="94"/>
        <source>Your data has been encrypted. Only the intended recipients can decrypt and read it.</source>
        <translation>Ваши данные зашифрованы. Только предназначенные получатели могут расшифровать и прочитать их.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="99"/>
        <source>Encryption failed: %1.</source>
        <translation>Не удалось зашифровать: %1.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgKeyTableModel</name>
    <message>
        <source>Select</source>
        <translation type="vanished">Выбрать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="43"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="43"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="44"/>
        <source>Email Address</source>
        <translation>Адрес электронной почты</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="44"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="253"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="44"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="254"/>
        <source>Trust</source>
        <translation>Доверие</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="45"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="45"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="45"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="46"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="46"/>
        <source>Subkey(s)</source>
        <translation>Подключи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="46"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="255"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="47"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="114"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="174"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="251"/>
        <source>ID</source>
        <translation>ID</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="252"/>
        <source>Algo</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="256"/>
        <source>No Comment</source>
        <translation>Нет комментария</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="262"/>
        <source>SubKeys (up to 8):</source>
        <translation>Подключи (до 8):</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="268"/>
        <source>ID: %1 | Algo: %2 | Usage: %3</source>
        <translation>ID: %1 | Алгоритм: %2 | Использование: %3</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgKeyTreeModel</name>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="41"/>
        <source>Select</source>
        <translation>Выбрать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="42"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="43"/>
        <source>Identity</source>
        <translation>Идентичность</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="44"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="45"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="46"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="47"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgSignResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="44"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="47"/>
        <source>Sign Operation</source>
        <translation>Операция подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="50"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="52"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="55"/>
        <source>Detail</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="66"/>
        <source>New Signature</source>
        <translation>Новая подпись</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="69"/>
        <source>Sign Mode</source>
        <translation>Режим подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="89"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="103"/>
        <source>Signed By</source>
        <translation>Подписано</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="93"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="96"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="94"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="97"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="99"/>
        <source>Key Create Date</source>
        <translation>Дата создания ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="103"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="139"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="104"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;неизвестно&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="107"/>
        <source>Public Key Algo</source>
        <translation>Алгоритм открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="109"/>
        <source>Hash Algo</source>
        <translation>Алгоритм хеширования</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="111"/>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="115"/>
        <source>Sign Date</source>
        <translation>Дата подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="111"/>
        <source>UTC</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="115"/>
        <source>Localized</source>
        <translation>Локализовано</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="131"/>
        <source>Invalid Signers</source>
        <translation>Недопустимые подписанты</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="137"/>
        <source>Signer</source>
        <translation>Подписант</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="142"/>
        <source>Reason</source>
        <translation>Причина</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="156"/>
        <source>Signed by %1. Recipients can verify this data came from you and was not altered.</source>
        <translation>Подписано %1. Получатели могут проверить, что эти данные отправлены вами и не были изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="158"/>
        <source>, </source>
        <translation>,</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="161"/>
        <source>A digital signature has been created. Recipients can verify this data came from you and was not altered.</source>
        <translation>Цифровая подпись создана. Получатели могут проверить, что эти данные отправлены вами и не были изменены.</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="167"/>
        <source>Signing completed, but %n signer(s) could not be used. Please review the details.</source>
        <translation>
            <numerusform>Подписание завершено, но %n подписант(ов) не удалось использовать. Пожалуйста, проверьте детали.</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="172"/>
        <source>Signing completed with warnings. Please review the details.</source>
        <translation>Подписание завершено с предупреждениями. Пожалуйста, проверьте детали.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgSignResultAnalyse.cpp" line="177"/>
        <source>Signing failed: %1.</source>
        <translation>Подписание не выполнено: %1.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgVerifyResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="45"/>
        <source>Verify Operation</source>
        <translation>Операция проверки</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="48"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="50"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="63"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="68"/>
        <source>Signed On</source>
        <translation>Подписано</translation>
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
        <translation>Локализовано</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="74"/>
        <source>Signatures List</source>
        <translation>Список подписей</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="81"/>
        <source>Signature [%1]:</source>
        <translation>Подпись [%1]:</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="82"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="132"/>
        <source>Tips</source>
        <translation>Подсказки</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="215"/>
        <source>Could not find information that can be used for verification.</source>
        <translation>Не удалось найти информацию, которую можно использовать для проверки.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="363"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="389"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="402"/>
        <source>Signed By</source>
        <translation>Подписано</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="363"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="402"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="365"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="403"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;неизвестно&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="368"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="408"/>
        <source>Public Key Algo</source>
        <translation>Алгоритм открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="370"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="410"/>
        <source>Hash Algo</source>
        <translation>Алгоритм хеширования</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="371"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="374"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="411"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="414"/>
        <source>Sign Date</source>
        <translation>Дата подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="140"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="392"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="395"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="42"/>
        <source>Verify</source>
        <translation>Проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="53"/>
        <source>Detail</source>
        <translation>Сведения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="85"/>
        <source>Invalid Signature</source>
        <translation>Недействительная подпись</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="87"/>
        <source>This signature could not be verified</source>
        <translation>Эту подпись не удалось проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="99"/>
        <source>Fully Valid</source>
        <translation>Полностью действителен</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="101"/>
        <source>Valid but with Issues</source>
        <translation>Действительна, но с проблемами</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="103"/>
        <source>Valid but Not Fully Trusted</source>
        <translation>Действительна, но не полностью доверена</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="109"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="257"/>
        <source>Signature has expired</source>
        <translation>Срок действия подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="112"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="259"/>
        <source>Signing key is missing</source>
        <translation>Ключ подписи отсутствует</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="115"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="261"/>
        <source>Signing key has been revoked</source>
        <translation>Ключ подписи был отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="118"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="263"/>
        <source>Signing key has expired</source>
        <translation>Срок действия ключа подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="121"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="265"/>
        <source>Certificate revocation list is missing</source>
        <translation>Отсутствует список отзыва сертификатов</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="125"/>
        <source>Warnings</source>
        <translation>Предупреждения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="133"/>
        <source>Adjust trust level to make it fully valid</source>
        <translation>Настройте уровень доверия для полной валидности</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="141"/>
        <source>not present in keyring</source>
        <translation>отсутствует в связке ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="148"/>
        <source>Cannot Verify due to Key Missing</source>
        <translation>Невозможно проверить из-за отсутствия ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="150"/>
        <source>The public key needed to verify this signature is not available</source>
        <translation>Открытый ключ, необходимый для проверки этой подписи, недоступен</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="159"/>
        <source>Key Revoked</source>
        <translation>Ключ отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="161"/>
        <source>The signature is mathematically valid but the signing key has been revoked</source>
        <translation>Подпись математически верна, но ключ подписи был отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="170"/>
        <source>Signature Expired</source>
        <translation>Срок действия подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="171"/>
        <source>The signature is valid but has expired</source>
        <translation>Подпись верна, но срок её действия истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="179"/>
        <source>Signing Key Expired</source>
        <translation>Срок действия ключа подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="182"/>
        <source>The signature is valid but the signing key has expired</source>
        <translation>Подпись верна, но срок действия ключа подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="189"/>
        <source>Verification Error</source>
        <translation>Ошибка проверки</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="191"/>
        <source>An error occurred during signature verification</source>
        <translation>Произошла ошибка при проверке подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="198"/>
        <source>Unknown Error</source>
        <translation>Неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="199"/>
        <source>Key Fingerprint</source>
        <translation>Отпечаток ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="206"/>
        <source>Verification process stopped due to errors.</source>
        <translation>Процесс проверки остановлен из-за ошибок.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="219"/>
        <source>No verifiable signatures were found in the data.</source>
        <translation>В данных не обнаружено подписей для проверки.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="311"/>
        <source>The signature is fully valid and trusted. The data has not been tampered with.</source>
        <translation>Подпись полностью действительна и доверена. Данные не были изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="313"/>
        <source>The signature by %1 is fully valid and trusted. The data has not been tampered with.</source>
        <translation>Подпись от %1 полностью действительна и доверена. Данные не были изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="318"/>
        <source>The signature is valid, but the signing key is not fully trusted. You may need to set a higher trust level.</source>
        <translation>Подпись действительна, но ключ подписи не полностью доверен. Возможно, потребуется повысить уровень доверия.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="324"/>
        <source>Verification incomplete. The signing key is not in your keyring. Please import the signer&apos;s public key.</source>
        <translation>Проверка не завершена. Ключа подписи нет в вашей связке ключей. Импортируйте открытый ключ подписанта.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="328"/>
        <source>The signature is valid but the signing key is not fully trusted. Consider setting a higher trust level for that key.</source>
        <translation>Подпись действительна, но ключ подписи не полностью доверен. Рекомендуется повысить уровень доверия для этого ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="332"/>
        <source>Verification completed with warnings. Please review the details.</source>
        <translation>Проверка завершена с предупреждениями. Ознакомьтесь с подробностями.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="338"/>
        <source>Invalid signature detected. The data may have been tampered with or the signature is corrupt.</source>
        <translation>Обнаружена недействительная подпись. Данные могли быть изменены или подпись повреждена.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="342"/>
        <source>Signature verification failed. The signing key has been revoked.</source>
        <translation>Проверка подписи не удалась. Ключ подписи отозван.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="346"/>
        <source>Signature verification failed. This signature has expired.</source>
        <translation>Проверка подписи не удалась. Срок действия подписи истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="349"/>
        <source>Signature verification failed. The signing key has expired.</source>
        <translation>Проверка подписи не удалась. Срок действия ключа подписи истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="352"/>
        <source>Cannot verify. The signing key is not available in your keyring.</source>
        <translation>Невозможно проверить. Ключ подписи отсутствует в вашей связке ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="356"/>
        <source>Signature verification failed. Please review the details.</source>
        <translation>Проверка подписи не удалась. Ознакомьтесь с подробностями.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="393"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="396"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="398"/>
        <source>Key Create Date</source>
        <translation>Дата создания ключа</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::KeyGenerateInfo</name>
    <message>
        <location filename="../../../../src/core/model/GpgKeyGenerateInfo.cpp" line="186"/>
        <source>None</source>
        <translation>Нет</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::Module::GlobalRegisterTableTreeModel::Impl</name>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="236"/>
        <source>Leaf</source>
        <translation>Лист</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="236"/>
        <source>Namespace</source>
        <translation>Пространство имён</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="264"/>
        <source>Empty</source>
        <translation>Пусто</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="268"/>
        <source>String</source>
        <translation>Строка</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="270"/>
        <source>Boolean</source>
        <translation>Логический</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="275"/>
        <source>Integer</source>
        <translation>Целое число</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="278"/>
        <source>Number</source>
        <translation>Число</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="280"/>
        <source>Unsupported</source>
        <translation>Не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="323"/>
        <source>&lt;UNSUPPORTED&gt;</source>
        <translation>&lt;НЕ ПОДДЕРЖИВАЕТСЯ&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="358"/>
        <source>Key</source>
        <translation>Ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="360"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="362"/>
        <source>Value Type</source>
        <translation>Тип значения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="364"/>
        <source>Value</source>
        <translation>Значение</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="366"/>
        <source>Version</source>
        <translation>Версия</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI</name>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="531"/>
        <source>Short Name</source>
        <translation>Короткое имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="532"/>
        <source>The name &quot;%1&quot; is shorter than five characters. Short names are allowed, but they are often a typo and make the key harder for others to recognise.

Do you want to continue?</source>
        <translation>Имя &quot;%1&quot; короче пяти символов. Короткие имена допускаются, но часто являются опечаткой и затрудняют распознавание ключа другим пользователям.

Вы хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="547"/>
        <source>Signature (OPENPGP.1)</source>
        <translation>Подпись (OPENPGP.1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="550"/>
        <source>Encryption (OPENPGP.2)</source>
        <translation>Шифрование (OPENPGP.2)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="553"/>
        <source>Authentication (OPENPGP.3)</source>
        <translation>Аутентификация (OPENPGP.3)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="572"/>
        <source>Select Card Slot</source>
        <translation>Выберите слот карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="573"/>
        <source>This key can be stored in more than one slot. Where should it be stored?</source>
        <translation>Этот ключ можно сохранить более чем в одном слоте. Где его следует сохранить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="594"/>
        <source>No Smart Card</source>
        <translation>Нет смарт-карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="595"/>
        <source>No OpenPGP smart card was detected. Insert a card and try again.</source>
        <translation>Смарт-карта OpenPGP не обнаружена. Вставьте карту и повторите попытку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="605"/>
        <source>Select Smart Card</source>
        <translation>Выберите смарт-карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="606"/>
        <source>Move the key to which card?</source>
        <translation>На какую карту переместить ключ?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="621"/>
        <source>Back Up Secret Key</source>
        <translation>Создать резервную копию секретного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="622"/>
        <source>Do you want to export a backup of the secret key before moving it to the card? After the move the key can only be used through the card.</source>
        <translation>Вы хотите экспортировать резервную копию секретного ключа перед перемещением на карту? После перемещения ключ можно будет использовать только через карту.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="627"/>
        <source>Back Up First</source>
        <translation>Сначала создать резервную копию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="630"/>
        <source>Continue Without Backup</source>
        <translation>Продолжить без резервной копии</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="652"/>
        <source>Export Secret Key Backup</source>
        <translation>Экспорт резервной копии секретного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="655"/>
        <source>Key Files</source>
        <translation>Файлы ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="661"/>
        <source>Export Error</source>
        <translation>Ошибка экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="662"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="684"/>
        <source>Move Key to Smart Card</source>
        <translation>Переместить ключ на смарт-карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="688"/>
        <source>&lt;h3&gt;You are about to move a private key onto a smart card.&lt;/h3&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br/&gt;&lt;br/&gt;This &lt;b&gt;moves&lt;/b&gt; the key: its private part is removed from this computer and only a card reference (stub) remains. Afterwards the key can only be used through the card. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br/&gt;&lt;br/&gt;Do you want to continue?</source>
        <translation>&lt;h3&gt;Вы собираетесь переместить закрытый ключ на смарт-карту.&lt;/h3&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br/&gt;&lt;br/&gt;Это &lt;b&gt;перемещает&lt;/b&gt; ключ: его закрытая часть удаляется с этого компьютера, и остаётся только ссылка на карту (заглушка). После этого ключ можно будет использовать только через карту. Это действие &lt;b&gt;необратимо&lt;/b&gt;.&lt;br/&gt;&lt;br/&gt;Вы хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="711"/>
        <source>This key has no capability that can be stored on a smart card.</source>
        <translation>У этого ключа нет возможностей, которые можно сохранить на смарт-карте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="739"/>
        <source>The key was moved to the smart card successfully.</source>
        <translation>Ключ был успешно перемещён на смарт-карту.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ADSKsPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="58"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="59"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="70"/>
        <source>No Subkeys Selected</source>
        <translation>Подключи не выбраны</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="71"/>
        <source>Please select at least one Subkey.</source>
        <translation>Выберите хотя бы один подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="81"/>
        <source>Select ADSK(s)</source>
        <translation>Выберите ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="86"/>
        <source>ADSK (Additional Decryption Subkey) allows others to encrypt data for you without having access to your private key. You are only allow to check subkeys with encryption capability.</source>
        <translation>ADSK (дополнительный подключ расшифровки) позволяет другим шифровать данные для вас без доступа к вашему закрытому ключу. Допускается выбор только подключей с возможностью шифрования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="110"/>
        <source>ADSKs Picker</source>
        <translation>Выбор ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="130"/>
        <source>Key ID: %1 Reason: %2</source>
        <translation>Идентификатор ключа: %1 Причина: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="142"/>
        <source>Unknown Error</source>
        <translation>Неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="143"/>
        <source>Caught unknown error while exporting the key.</source>
        <translation>Обнаружена неизвестная ошибка при экспорте ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="160"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="161"/>
        <source>Partially Failed</source>
        <translation>Частично не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="164"/>
        <source>Failed to add all selected subkeys.</source>
        <translation>Не удалось добавить все выбранные подключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="165"/>
        <source>Some subkeys failed to be added as ADSKs.</source>
        <translation>Не удалось добавить некоторые подключи как ADSK.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AboutDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="253"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="263"/>
        <source>About</source>
        <translation>О программе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="264"/>
        <source>Build Information</source>
        <translation>Сведения о сборке</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="266"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="655"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="269"/>
        <source>Rust Engine</source>
        <translation>Rust-движок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="265"/>
        <source>Translators</source>
        <translation>Переводчики</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AdvancedTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="98"/>
        <source>Security</source>
        <translation>Безопасность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="113"/>
        <source>wipe freed memory</source>
        <translation>очищать освобождённую память</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="115"/>
        <source>also lock memory pages</source>
        <translation>также блокировать страницы памяти</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="117"/>
        <source>also rotate keys weekly</source>
        <translation>также еженедельно ротировать ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="119"/>
        <source>no extra hardening</source>
        <translation>без дополнительного усиления</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="129"/>
        <source>How aggressively the application protects your secrets. Higher levels cost some performance.</source>
        <translation>Насколько агрессивно приложение защищает ваши секреты. Более высокие уровни снижают производительность.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="131"/>
        <source>Secure Level:</source>
        <translation>Уровень защиты:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="137"/>
        <source>No extra protection (default)</source>
        <translation>Без дополнительной защиты (по умолчанию)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="140"/>
        <source>System keychain</source>
        <translation>Системная связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="142"/>
        <source>PIN at startup</source>
        <translation>PIN при запуске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="145"/>
        <source>How the application key file is protected on disk. The system keychain keeps a secret on this computer and never asks you for a password; a PIN is asked for each time the application starts.</source>
        <translation>Как файл ключа приложения защищается на диске. Системная связка ключей хранит секрет на этом компьютере и никогда не запрашивает пароль; PIN запрашивается при каждом запуске приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="149"/>
        <source>Change PIN…</source>
        <translation>Изменить PIN…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="154"/>
        <source>Application Key Protection:</source>
        <translation>Защита ключа приложения:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="157"/>
        <source>Weekly key rotation offers little protection while the application key itself is stored unprotected on disk. Consider using the system keychain or a PIN.</source>
        <translation>Еженедельная ротация ключей обеспечивает слабую защиту, если сам ключ приложения хранится на диске без защиты. Рекомендуется использовать системную связку ключей или PIN-код.</translation>
    </message>
    <message>
        <source>Verify signed libraries and binaries at startup</source>
        <translation type="vanished">Проверять подписанные библиотеки и исполняемые файлы при запуске</translation>
    </message>
    <message>
        <source>Check that the shipped libraries and executables still match the signatures made at build time. The application refuses to start if the check fails.</source>
        <translation type="vanished">Проверяет, что поставляемые библиотеки и исполняемые файлы по-прежнему соответствуют подписям, созданным во время сборки. Приложение откажется запускаться, если проверка не пройдена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="165"/>
        <source>Diagnostics</source>
        <translation>Диагностика</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="169"/>
        <source>Debug</source>
        <translation>Отладка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="170"/>
        <source>Info</source>
        <translation>Информация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="171"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="173"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="175"/>
        <source>Fatal</source>
        <translation>Критическая</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="177"/>
        <source>The least severe message that still gets written to the log. Debug is the most detailed and writes the most to disk.</source>
        <translation>Наименее критичное сообщение, которое всё ещё записывается в журнал. Отладка — самый подробный уровень, записывающий больше всего данных на диск.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="179"/>
        <source>Log Level:</source>
        <translation>Уровень журнала:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="183"/>
        <source> entries</source>
        <translation>записи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="185"/>
        <source>How many recent log messages are kept in memory for crash reports and the log viewer. Larger values use more memory.</source>
        <translation>Сколько последних сообщений журнала хранится в памяти для отчетов о сбоях и просмотра журнала. Большие значения потребляют больше памяти.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="187"/>
        <source>Log Ring Buffer:</source>
        <translation>Кольцевой буфер журнала:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="190"/>
        <source>These settings are read once while the application starts, so a change only takes effect after a restart.</source>
        <translation>Эти параметры считываются один раз при запуске приложения, поэтому изменения вступают в силу только после перезапуска.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="196"/>
        <source>Some settings on this page are fixed by the ENV.ini file next to the application and cannot be changed here. Edit that file to change them.</source>
        <translation>Некоторые параметры на этой странице задаются файлом ENV.ini рядом с приложением и не могут быть изменены здесь. Отредактируйте этот файл, чтобы изменить их.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="245"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="501"/>
        <source>System Keychain Unavailable</source>
        <translation>Системная связка ключей недоступна</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="246"/>
        <source>The system credential store could not be used, so the application key cannot be protected with it.</source>
        <translation>Не удалось использовать системное хранилище учетных данных, поэтому ключ приложения не может быть с его помощью защищен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="249"/>
        <source>On Linux this needs a running secret service, such as GNOME Keyring or KWallet, and it must be unlocked.</source>
        <translation>В Linux для этого требуется работающий сервис секретов, например, GNOME Keyring или KWallet, и он должен быть разблокирован.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="282"/>
        <source>Turn Off Weekly Key Rotation?</source>
        <translation>Отключить еженедельную смену ключа?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="283"/>
        <source>At the %1 level the application saves your data with a key that changes every week. Choosing a lower level stops loading those keys, so anything saved while this level was on can no longer be read and is deleted after a short grace period.</source>
        <translation>На уровне %1 приложение сохраняет ваши данные с помощью ключа, который меняется каждую неделю. Выбор более низкого уровня останавливает загрузку этих ключей, поэтому все, что было сохранено, пока этот уровень был включен, больше не может быть прочитано и будет удалено после короткого льготного периода.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="289"/>
        <source>Lower the level anyway?</source>
        <translation>Все равно понизить уровень?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="316"/>
        <source>Fixed by ENV.ini and cannot be changed here.</source>
        <translation>Задано файлом ENV.ini и не может быть изменено здесь.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="388"/>
        <source>This profile was opened from a file, and the passphrase that opens that file also protects its key. To change it, export the profile again with a new passphrase.</source>
        <translation>Этот профиль был открыт из файла, и парольная фраза, открывающая этот файл, также защищает его ключ. Чтобы изменить её, экспортируйте профиль снова с новой парольной фразой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="403"/>
        <source>Not available for a profile file: the file is meant to be opened on another computer, possibly running another operating system, where a secret stored in this computer&apos;s credential store cannot be read. Use a PIN, or no protection.</source>
        <translation>Недоступно для файла профиля: файл предназначен для открытия на другом компьютере, возможно, с другой операционной системой, где секрет, хранящийся в хранилище учётных данных этого компьютера, не может быть прочитан. Используйте PIN или не используйте защиту.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="408"/>
        <source>Not available in portable mode: a portable installation must not depend on secrets stored on one particular computer.</source>
        <translation>Недоступно в портативном режиме: портативная установка не должна зависеть от секретов, хранящихся на одном конкретном компьютере.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="416"/>
        <source>No system credential store is available on this computer. On Linux this needs a running secret service, such as GNOME Keyring or KWallet, and the libsecret client library (libsecret-1-0 on Debian and Ubuntu, libsecret on Fedora).</source>
        <translation>На этом компьютере нет системного хранилища учётных данных. В Linux для этого требуется работающая служба секретов, например GNOME Keyring или KWallet, и клиентская библиотека libsecret (libsecret-1-0 в Debian и Ubuntu, libsecret в Fedora).</translation>
    </message>
    <message>
        <source>No system credential store is available on this computer. On Linux this needs a running secret service, such as GNOME Keyring or KWallet.</source>
        <translation type="vanished">На этом компьютере нет системного хранилища учётных данных. В Linux для этого требуется работающая служба секретов, например GNOME Keyring или KWallet.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="502"/>
        <source>The application key could not be protected with the system keychain, so it has been left exactly as it was.</source>
        <translation>Не удалось защитить ключ приложения с помощью системной связки ключей, поэтому он остался в исходном состоянии.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="506"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="551"/>
        <source>Application Key Error</source>
        <translation>Ошибка ключа приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="507"/>
        <source>The application key could not be re-protected: %1.</source>
        <translation>Не удалось повторно защитить ключ приложения: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="510"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="553"/>
        <source>It has been left exactly as it was, so nothing has been lost.</source>
        <translation>Он остался в исходном состоянии, поэтому ничего не потеряно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="534"/>
        <source>The current PIN is not correct.</source>
        <translation>Текущий PIN-код неверен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="545"/>
        <source>PIN Changed</source>
        <translation>PIN-код изменён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="546"/>
        <source>The application PIN has been changed.</source>
        <translation>PIN-код приложения был изменён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="552"/>
        <source>The PIN could not be changed: %1.</source>
        <translation>Не удалось изменить PIN-код: %1.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AppKeyPinDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="63"/>
        <source>Unlock Application Key</source>
        <translation>Разблокировать ключ приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="65"/>
        <source>Change Application PIN</source>
        <translation>Изменить PIN приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="67"/>
        <source>Set an Application PIN</source>
        <translation>Установить PIN приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="92"/>
        <source>This PIN encrypts the application key on disk. You will be asked for it every time the application starts.</source>
        <translation>Этот PIN шифрует ключ приложения на диске. Он будет запрашиваться при каждом запуске приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="94"/>
        <source>This application&apos;s key is protected by a PIN. Enter it to continue.</source>
        <translation>Ключ этого приложения защищён PIN. Введите его для продолжения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="162"/>
        <source>Current PIN</source>
        <translation>Текущий PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="162"/>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="168"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="168"/>
        <source>New PIN</source>
        <translation>Новый PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="172"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="179"/>
        <source>Show PIN</source>
        <translation>Показать PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="192"/>
        <source>Strength</source>
        <translation>Сложность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="217"/>
        <source>If you forget this PIN, everything the application has encrypted becomes permanently unreadable. There is no recovery.</source>
        <translation>Если вы забудете этот PIN, всё, что зашифровало приложение, станет навсегда недоступным для чтения. Восстановление невозможно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="244"/>
        <source>This PIN cannot be recovered if it is lost.</source>
        <translation>Этот PIN невозможно восстановить в случае утери.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="245"/>
        <source>Use at least %1 characters.</source>
        <translation>Используйте не менее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="252"/>
        <source>Unlock</source>
        <translation>Разблокировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="252"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="260"/>
        <source>Quit</source>
        <translation>Выйти</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="269"/>
        <source>Forgot PIN? Reset…</source>
        <translation>Забыли PIN? Сбросить…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="391"/>
        <source>The PIN must be at least %1 characters.</source>
        <translation>PIN должен содержать не менее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="393"/>
        <source>The two PINs do not match.</source>
        <translation>Введённые PIN-коды не совпадают.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AppearanceTab</name>
    <message>
        <source>General</source>
        <translation type="vanished">Общие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="78"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="79"/>
        <source>Theme</source>
        <translation>Тема</translation>
    </message>
    <message>
        <source>Toolbar Icon</source>
        <translation type="vanished">Значки панели инструментов</translation>
    </message>
    <message>
        <source>Size</source>
        <translation type="vanished">Размер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="81"/>
        <source>Toolbar</source>
        <translation>Панель инструментов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="83"/>
        <source>Icon Size</source>
        <translation>Размер значков</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="84"/>
        <source>small</source>
        <translation>маленький</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="85"/>
        <source>medium</source>
        <translation>средний</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="86"/>
        <source>large</source>
        <translation>большой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="88"/>
        <source>Icon Style</source>
        <translation>Стиль значков</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="89"/>
        <source>just text</source>
        <translation>только текст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="90"/>
        <source>just icons</source>
        <translation>только значки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="91"/>
        <source>text and icons</source>
        <translation>текст и значки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="93"/>
        <source>Actions</source>
        <translation>Действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="95"/>
        <source>IM actions turn the text into one compact line that is safe to paste into an instant messenger.</source>
        <translation>Действия IM преобразуют текст в одну компактную строку, которую можно безопасно вставить в мессенджер.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="99"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="110"/>
        <source>Font Family</source>
        <translation>Семейство шрифтов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="101"/>
        <source>Tab Size</source>
        <translation>Размер табуляции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="103"/>
        <source>Show all fonts</source>
        <translation>Показать все шрифты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="105"/>
        <source>Also offer proportional fonts for both surfaces below. They line up best with a monospaced one, but scripts such as Arabic are only shaped correctly by a font that covers them.</source>
        <translation>Также предлагает пропорциональные шрифты для обеих областей ниже. Лучше всего они выравниваются с моноширинным шрифтом, но такие письменности, как арабская, правильно отображаются только шрифтом, который их поддерживает.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="109"/>
        <source>Status Panel</source>
        <translation>Панель состояния</translation>
    </message>
    <message>
        <source>Style</source>
        <translation type="vanished">Стиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="100"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="111"/>
        <source>Font Size</source>
        <translation>Размер шрифта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="98"/>
        <source>Text Editor</source>
        <translation>Текстовый редактор</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::BuildInfoTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="444"/>
        <source>GpgFrontend:</source>
        <translation>GpgFrontend:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="447"/>
        <source>Qt:</source>
        <translation>Qt:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="448"/>
        <source>GPGME:</source>
        <translation>GPGME:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="450"/>
        <source>Assuan:</source>
        <translation>Assuan:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="453"/>
        <source>Libarchive:</source>
        <translation>Libarchive:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="456"/>
        <source>OpenSSL:</source>
        <translation>OpenSSL:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="458"/>
        <source>Sodium:</source>
        <translation>Sodium:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="461"/>
        <source>Git Branch:</source>
        <translation>Ветка Git:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="464"/>
        <source>Git Commit:</source>
        <translation>Коммит Git:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="467"/>
        <source>Built at:</source>
        <translation>Собрано:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="471"/>
        <source>Copy Build Information</source>
        <translation>Копировать информацию о сборке</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="478"/>
        <source>Build Information</source>
        <translation>Информация о сборке</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ChoosePage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="411"/>
        <source>Choose a guide</source>
        <translation>Выберите руководство</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="412"/>
        <source>Pick a topic if you want to learn the basics first.</source>
        <translation>Выберите тему, если хотите сначала изучить основы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="414"/>
        <source>Generate a new Key Pair</source>
        <translation>Создать новую пару ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="415"/>
        <source>Learn how to create your own key pairs.</source>
        <translation>Узнайте, как создавать собственные пары ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="418"/>
        <source>Text operations</source>
        <translation>Текстовые операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="419"/>
        <source>Learn how to encrypt, decrypt, sign, and verify text messages.</source>
        <translation>Узнайте, как шифровать, расшифровывать, подписывать и проверять текстовые сообщения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="422"/>
        <source>File operations</source>
        <translation>Файловые операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="423"/>
        <source>Learn how to encrypt, decrypt, sign, and verify files securely.</source>
        <translation>Узнайте, как безопасно шифровать, расшифровывать, подписывать и проверять файлы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="426"/>
        <source>View key pair information</source>
        <translation>Просмотр информации о паре ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="427"/>
        <source>Learn how to inspect key details, user IDs, fingerprints, and key capabilities.</source>
        <translation>Узнайте, как просматривать сведения о ключе, идентификаторы пользователей, отпечатки и возможности ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="431"/>
        <source>You can also skip these guides and start using GpgFrontend directly.</source>
        <translation>Вы также можете пропустить эти руководства и сразу начать использовать GpgFrontend.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::CommonUtils</name>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="105"/>
        <source>No Supported OpenPGP Engine Found</source>
        <translation>Поддерживаемый движок OpenPGP не найден</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="106"/>
        <source>It seems that no supported OpenPGP engine is available. Please check your if GpgFrontend is properly installed and try again. Reason: %1</source>
        <translation>Похоже, что поддерживаемый движок OpenPGP недоступен. Пожалуйста, проверьте правильность установки GpgFrontend и попробуйте снова. Причина: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="235"/>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="380"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="244"/>
        <source>Operation Not Supported</source>
        <translation>Операция не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="254"/>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="372"/>
        <source>Failure</source>
        <translation>Неудача</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="283"/>
        <source>Open Key</source>
        <translation>Открыть ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="284"/>
        <source>Keyring files</source>
        <translation>Файлы связки ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="291"/>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="298"/>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="426"/>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="471"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="292"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Невозможно открыть этот файл. Убедитесь, что это обычный файл и он доступен для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="299"/>
        <source>The target file is too large for a keyring.</source>
        <translation>Целевой файл слишком большой для связки ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="305"/>
        <source>File Open Failed</source>
        <translation>Не удалось открыть файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="306"/>
        <source>Failed to open file: </source>
        <translation>Не удалось открыть файл: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="353"/>
        <source>Processing</source>
        <translation>Обработка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="373"/>
        <source>Failed to execute command.</source>
        <translation>Не удалось выполнить команду.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="381"/>
        <source>Succeed in executing command.</source>
        <translation>Команда выполнена успешно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="383"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="384"/>
        <source>Finished executing command.</source>
        <translation>Выполнение команды завершено.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="471"/>
        <source>Key Not Found.</source>
        <translation>Ключ не найден.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="255"/>
        <source>Gpg Operation failed.</source>
        <translation>Операция GPG не удалась.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="74"/>
        <source>No Usable Key Database</source>
        <translation>Нет доступной базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="75"/>
        <source>None of the configured key databases could be opened. This usually means the folder was moved or deleted, or is on a drive that is not currently available.</source>
        <translation>Ни одну из настроенных баз данных ключей не удалось открыть. Обычно это означает, что папка была перемещена, удалена или находится на диске, который в данный момент недоступен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="79"/>
        <source>You can change where your key databases live in Settings, under Key Databases. Details: %1</source>
        <translation>Вы можете изменить расположение баз данных ключей в настройках, в разделе «Базы данных ключей». Подробности: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="86"/>
        <source>Cannot Prepare Application Data</source>
        <translation>Невозможно подготовить данные приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="87"/>
        <source>GpgFrontend could not set up the folders it needs to store its data. Please check that the application data folder is writable. Details: %1</source>
        <translation>GpgFrontend не удалось настроить папки, необходимые для хранения данных. Проверьте, что папка данных приложения доступна для записи. Подробности: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="94"/>
        <source>Key Database Could Not Be Opened</source>
        <translation>Не удалось открыть базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="95"/>
        <source>The key database was found but could not be loaded. It may be in use by another program, or its permissions may have changed. Details: %1</source>
        <translation>База данных ключей найдена, но не может быть загружена. Возможно, она используется другой программой или были изменены её права доступа. Подробности: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="236"/>
        <source>Operation completed successfully.</source>
        <translation>Операция успешно завершена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="245"/>
        <source>The current OpenPGP engine does not support this operation. Please use a supported engine or upgrade the engine version.</source>
        <translation>Текущий движок OpenPGP не поддерживает эту операцию. Пожалуйста, используйте поддерживаемый движок или обновите версию движка.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="256"/>
        <source>Error code: %1</source>
        <translation>Код ошибки: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="257"/>
        <source>Source:  %1</source>
        <translation>Источник:  %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="258"/>
        <source>Description: %1</source>
        <translation>Описание: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="259"/>
        <source>Error Message: %1</source>
        <translation>Сообщение об ошибке: %1</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ConclusionPage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="497"/>
        <source>Ready to use</source>
        <translation>Готово к использованию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="499"/>
        <source>GpgFrontend is ready. You can adjust these options before finishing.</source>
        <translation>GpgFrontend готов к работе. Вы можете изменить эти параметры перед завершением.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="503"/>
        <source>Contact and feedback</source>
        <translation>Контакты и обратная связь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="504"/>
        <source>Report issues, ask questions, or send feedback to help improve GpgFrontend.</source>
        <translation>Сообщайте о проблемах, задавайте вопросы или отправляйте отзывы, чтобы помочь улучшить GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="508"/>
        <source>Submit an issue on GitHub</source>
        <translation>Сообщить о проблеме на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="509"/>
        <source>Use GitHub issues if you want to report a bug or track a technical problem.</source>
        <translation>Используйте GitHub Issues, если хотите сообщить об ошибке или отследить техническую проблему.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="514"/>
        <source>Don&apos;t show this setup wizard again</source>
        <translation>Больше не показывать мастер настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="512"/>
        <source>Check for updates on startup</source>
        <translation>Проверять обновления при запуске</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::CreateFileSystemItemDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="68"/>
        <source>New Folder</source>
        <translation>Новая папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="69"/>
        <source>New File</source>
        <translation>Новый файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="77"/>
        <source>Create a new folder</source>
        <translation>Создать новую папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="78"/>
        <source>Create a new empty file</source>
        <translation>Создать новый пустой файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="89"/>
        <source>Location: %1</source>
        <translation>Расположение: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="94"/>
        <source>Folder name</source>
        <translation>Имя папки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="95"/>
        <source>File name, e.g. notes.txt</source>
        <translation>Имя файла, например notes.txt</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="102"/>
        <source>Name:</source>
        <translation>Имя:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="107"/>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="143"/>
        <source>Enter a folder name.</source>
        <translation>Введите имя папки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="144"/>
        <source>Enter a file name.</source>
        <translation>Введите имя файла.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="147"/>
        <source>This name is reserved.</source>
        <translation>Это имя зарезервировано.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="150"/>
        <source>The name must not contain path separators.</source>
        <translation>Имя не должно содержать разделителей пути.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="153"/>
        <source>This name is reserved by the operating system.</source>
        <translation>Это имя зарезервировано операционной системой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="156"/>
        <source>A file or folder with this name already exists.</source>
        <translation>Файл или папка с таким именем уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="160"/>
        <source>The folder will be created in the selected location.</source>
        <translation>Папка будет создана в выбранном расположении.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/CreateFileSystemItemDialog.cpp" line="161"/>
        <source>An empty file will be created in the selected location.</source>
        <translation>Пустой файл будет создан в выбранном расположении.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::DocViewerDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="93"/>
        <source>Document Preview</source>
        <translation>Предпросмотр документа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="94"/>
        <source>Document Preview — %1</source>
        <translation>Предпросмотр документа — %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="126"/>
        <source>Zoom out</source>
        <translation>Уменьшить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="130"/>
        <source>Zoom in</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="134"/>
        <source>Fit to window</source>
        <translation>Вписать в окно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="137"/>
        <source>Actual size (100%)</source>
        <translation>Настоящий размер (100%)</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::EncryptionKeysPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="80"/>
        <source>Select Encryption Key(s)</source>
        <translation>Выберите ключ(и) шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="85"/>
        <source>Choose Encryption Key(s) or Subkey(s)</source>
        <translation>Выберите ключ(и) или подключ(и) шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="91"/>
        <source>Select the recipient key(s) or specific encryption subkey(s) to use for this operation. Expand a key to choose a particular subkey.</source>
        <translation>Выберите ключ(и) получателя или конкретный(е) подключ(и) шифрования для использования в этой операции. Разверните ключ, чтобы выбрать конкретный подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="105"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="108"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="113"/>
        <source>No Key Selected</source>
        <translation>Ключ не выбран</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/EncryptionKeysPicker.cpp" line="114"/>
        <source>Please select at least one encryption key or subkey.</source>
        <translation>Пожалуйста, выберите хотя бы один ключ или подключ шифрования.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ExportKey</name>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="116"/>
        <source>Unknown Error</source>
        <translation>Неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="117"/>
        <source>Caught unknown error while exporting the key.</source>
        <translation>При экспорте ключа возникла неизвестная ошибка.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="135"/>
        <source>Export Key To File</source>
        <translation>Экспортировать ключ в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="136"/>
        <source>Key Files</source>
        <translation>Файлы ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="142"/>
        <source>Export Error</source>
        <translation>Ошибка экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="143"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="148"/>
        <source>Export Successful</source>
        <translation>Экспорт выполнен успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="149"/>
        <source>The key has been successfully exported to %1.</source>
        <translation>Ключ успешно экспортирован в %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="163"/>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="185"/>
        <source>WARNING: You are about to export your</source>
        <translation>ВНИМАНИЕ: Вы собираетесь экспортировать ваш</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="164"/>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="186"/>
        <source>PRIVATE KEY</source>
        <translation>ЗАКРЫТЫЙ КЛЮЧ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="165"/>
        <source>This is NOT your Public Key, so &lt;b&gt;DO NOT&lt;/b&gt; share it with anyone.</source>
        <translation>Это НЕ ваш открытый ключ, поэтому &lt;b&gt;НЕ&lt;/b&gt; делитесь им ни с кем.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="168"/>
        <source>You are exporting a &lt;b&gt;minimum size&lt;/b&gt; private key, which removes all signatures except for the latest self-signatures.</source>
        <translation>Вы экспортируете закрытый ключ &lt;b&gt;минимального размера&lt;/b&gt;, который удаляет все подписи, кроме последних самоподписей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="170"/>
        <source>Do you &lt;b&gt;REALLY&lt;/b&gt; want to proceed?</source>
        <translation>Вы &lt;b&gt;ДЕЙСТВИТЕЛЬНО&lt;/b&gt; хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="173"/>
        <source>Exporting Short Private Key</source>
        <translation>Экспорт сокращённого закрытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="187"/>
        <source>This operation will export your &lt;b&gt;private key&lt;/b&gt;, including both the main key and all subkeys, into an external file. This key is extremely sensitive, and anyone with access to it can impersonate you. DO NOT share this file with anyone!</source>
        <translation>Эта операция экспортирует ваш &lt;b&gt;закрытый ключ&lt;/b&gt;, включая основной ключ и все подключи, во внешний файл. Этот ключ крайне чувствителен, и любой, имеющий к нему доступ, может выдать себя за вас. НЕ делитесь этим файлом ни с кем!</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="193"/>
        <source>Are you &lt;b&gt;ABSOLUTELY SURE&lt;/b&gt; you want to proceed?</source>
        <translation>Вы &lt;b&gt;АБСОЛЮТНО УВЕРЕНЫ&lt;/b&gt;, что хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ExportKey.cpp" line="195"/>
        <source>Exporting Private Key</source>
        <translation>Экспорт закрытого ключа</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ExportKeyPackageDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="53"/>
        <source>Export Key Package</source>
        <translation>Экспортировать пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="54"/>
        <source>Key Package</source>
        <translation>Пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="64"/>
        <source>Export Key Package Passphrase</source>
        <translation>Пароль пакета ключей для экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="66"/>
        <source>Key File</source>
        <translation>Файл ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="97"/>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="143"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="76"/>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="83"/>
        <source>Forbidden</source>
        <translation>Запрещено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="77"/>
        <source>Please select an output path before exporting.</source>
        <translation>Выберите путь вывода перед экспортом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="84"/>
        <source>Please generate a password to protect your key before exporting, it is very important. Don&apos;t forget to back up your password in a safe place.</source>
        <translation>Сгенерируйте пароль для защиты ключа перед экспортом — это очень важно. Не забудьте сохранить пароль в надёжном месте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="98"/>
        <source>No key is suitable to export.</source>
        <translation>Нет подходящих ключей для экспорта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="103"/>
        <source>Enter PIN</source>
        <translation>Введите PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="104"/>
        <source>Please enter PIN to protect the Key:</source>
        <translation>Введите PIN для защиты ключа:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="113"/>
        <source>Generating</source>
        <translation>Генерация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="123"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="131"/>
        <source>But the key file cannot be leaked under any circumstances. Please delete the Key Package and key file as soon as possible after completing the transfer operation.</source>
        <translation>Файл ключа ни в коем случае не должен быть раскрыт. Удалите пакет ключей и файл ключа как можно скорее после завершения передачи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="125"/>
        <source>The Key Package has been successfully generated and has been protected by encryption algorithms(AES-256-GCM). You can safely transfer your Key Package.</source>
        <translation>Пакет ключей успешно сгенерирован и защищён алгоритмом шифрования (AES-256-GCM). Вы можете безопасно передать свой пакет ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="152"/>
        <source>Key Package Name</source>
        <translation>Имя пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="153"/>
        <source>Output Path</source>
        <translation>Путь вывода</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="154"/>
        <source>Passphrase</source>
        <translation>Парольная фраза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="156"/>
        <source>Tips: You can use Key Package to safely and conveniently transfer your public and private keys between devices.</source>
        <translation>Подсказка: вы можете использовать пакет ключей для безопасной и удобной передачи открытых и закрытых ключей между устройствами.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="158"/>
        <source>Generate and Save Passphrase</source>
        <translation>Сгенерировать и сохранить парольную фразу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="159"/>
        <source>Generate Key Package Name</source>
        <translation>Сгенерировать имя пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="160"/>
        <source>Select Output Path</source>
        <translation>Выберите путь вывода</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="163"/>
        <source>Include secret key (Think twice before acting)</source>
        <translation>Включить секретный ключ (подумайте дважды перед действием)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="165"/>
        <source>Exclude keys that do not have a private key</source>
        <translation>Исключить ключи без закрытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="168"/>
        <source>Export As Key Package</source>
        <translation>Экспортировать как пакет ключей</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FilePage</name>
    <message>
        <source>Show Hidden Files</source>
        <translation type="vanished">Показывать скрытые файлы</translation>
    </message>
    <message>
        <source>Show System Files</source>
        <translation type="vanished">Показывать системные файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="165"/>
        <source>Use ASCII Armor</source>
        <translation>Использовать ASCII-броню</translation>
    </message>
    <message>
        <source>Use ASCII armored output for file operations.</source>
        <translation type="vanished">Использовать вывод в формате ASCII-брони для файловых операций.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="108"/>
        <source>New Folder</source>
        <translation>Новая папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="109"/>
        <source>Create a new folder in the current folder.</source>
        <translation>Создать новую папку в текущей папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="116"/>
        <source>New Empty File</source>
        <translation>Новый пустой файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="117"/>
        <source>Create an empty file in the current folder.</source>
        <translation>Создать пустой файл в текущей папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="130"/>
        <source>Show</source>
        <translation>Показывать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="132"/>
        <source>Choose what the file list shows.</source>
        <translation>Выберите, что отображать в списке файлов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="135"/>
        <source>Hidden Files</source>
        <translation>Скрытые файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="138"/>
        <source>List files and folders whose name starts with a dot.</source>
        <translation>Показывать файлы и папки, имена которых начинаются с точки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="143"/>
        <source>System Files</source>
        <translation>Системные файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="146"/>
        <source>List system files such as devices and sockets.</source>
        <translation>Показывать системные файлы, такие как устройства и сокеты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="151"/>
        <source>Type Column</source>
        <translation>Столбец типа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="153"/>
        <source>Show the file type as its own column.</source>
        <translation>Показывать тип файла в отдельном столбце.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="167"/>
        <source>Write the result of encrypting or signing as printable text (.asc) instead of binary.</source>
        <translation>Записывать результат зашифрования или подписания в виде печатного текста (.asc) вместо двоичного.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="208"/>
        <source>Batch mode is enabled. No file is selected.</source>
        <translation>Включен пакетный режим. Файлы не выбраны.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="209"/>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="228"/>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="267"/>
        <source>Enable batch mode to select multiple files.</source>
        <translation>Включите пакетный режим для выбора нескольких файлов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="214"/>
        <source>%1 item(s) selected.</source>
        <translation>Выбрано элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="227"/>
        <source>Batch mode is enabled. Multiple files can be selected.</source>
        <translation>Включен пакетный режим. Можно выбрать несколько файлов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="237"/>
        <source>Filter</source>
        <translation>Фильтр</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="239"/>
        <source>List only the files and folders whose name contains this text. The filter applies to the current folder and is cleared when you open another one.</source>
        <translation>Показывать только файлы и папки, в имени которых содержится этот текст. Фильтр применяется к текущей папке и сбрасывается при открытии другой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="260"/>
        <source>Go to the parent folder (Backspace)</source>
        <translation>Перейти в родительскую папку (Backspace)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="261"/>
        <source>Read this folder from disk again</source>
        <translation>Обновить содержимое этой папки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="263"/>
        <source>Go to a mounted volume or removable drive</source>
        <translation>Перейти к смонтированному тому или сменному диску</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="265"/>
        <source>Create items and choose what the list shows</source>
        <translation>Создавайте элементы и выбирайте, что отображается в списке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="295"/>
        <source>Entries listed in this folder, and how many of them are selected. Entries hidden by the filter are not counted.</source>
        <translation>Записи в этой папке и количество выбранных. Записи, скрытые фильтром, не учитываются.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="298"/>
        <source>Space still available on the volume holding this folder.</source>
        <translation>Свободное место на томе, содержащем эту папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="433"/>
        <source>Profile Workspace</source>
        <translation>Рабочее пространство профиля</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="481"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n элемент
%n элемента
%n элементов
%n элементов</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="484"/>
        <source>%1 selected</source>
        <translation>%1 выбрано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="498"/>
        <source>%1 free</source>
        <translation>%1 свободно</translation>
    </message>
    <message>
        <source>Type a folder path, e.g. ~/Documents</source>
        <translation type="vanished">Введите путь к папке, например ~/Documents</translation>
    </message>
    <message>
        <source>Go to Parent Directory</source>
        <translation type="vanished">Перейти в родительскую директорию</translation>
    </message>
    <message>
        <source>Refresh</source>
        <translation type="vanished">Обновить</translation>
    </message>
    <message>
        <source>Create New Directory</source>
        <translation type="vanished">Создать новую директорию</translation>
    </message>
    <message>
        <source>Mounted Volumes</source>
        <translation type="vanished">Подключенные тома</translation>
    </message>
    <message>
        <source>File View Options</source>
        <translation type="vanished">Параметры отображения файлов</translation>
    </message>
    <message>
        <source>Enable Batch Mode</source>
        <translation type="vanished">Включить пакетный режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="317"/>
        <source>The folder does not exist or cannot be opened.</source>
        <translation>Папка не существует или не может быть открыта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="466"/>
        <source>No Available Volumes</source>
        <translation>Нет доступных томов</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FilePathBar</name>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="138"/>
        <source>Type a folder path, e.g. ~/Documents</source>
        <translation>Введите путь к папке, например ~/Документы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="140"/>
        <source>Type a folder path and press Enter. &quot;~&quot; stands for your home folder, and a relative path is resolved against the current one. Press Escape to go back to the path buttons.</source>
        <translation>Введите путь к папке и нажмите Enter. &quot;~&quot; обозначает вашу домашнюю папку, а относительный путь разрешается относительно текущей. Нажмите Escape, чтобы вернуться к кнопкам пути.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="166"/>
        <source>Click a folder in the path to go there, or click the edit button to type a path (Ctrl+L).</source>
        <translation>Нажмите на папку в пути, чтобы перейти туда, или нажмите кнопку редактирования, чтобы ввести путь (Ctrl+L).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="398"/>
        <source>Type a path instead (Ctrl+L)</source>
        <translation>Ввести путь вместо этого (Ctrl+L)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="418"/>
        <source>Show parent folders</source>
        <translation>Показать родительские папки</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FileTreeItemDelegate</name>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="110"/>
        <source>SIG</source>
        <extracomment>Badge on a detached OpenPGP signature file. Keep it to about three characters.</extracomment>
        <translation>ПОД</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="113"/>
        <source>ENC</source>
        <translation>ШИФ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="186"/>
        <source>Detached OpenPGP signature</source>
        <translation>Открепленная подпись OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeItemDelegate.cpp" line="190"/>
        <source>Encrypted or armored OpenPGP file</source>
        <translation>Зашифрованный или ASCII-бронированный файл OpenPGP</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FileTreeView</name>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="572"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="632"/>
        <source>Rename</source>
        <translation>Переименовать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="616"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="648"/>
        <source>Calculate Hash</source>
        <translation>Вычислить хеш</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="671"/>
        <source>Compress...</source>
        <translation>Сжать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="341"/>
        <source>Unable to Open File</source>
        <translation>Не удалось открыть файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="265"/>
        <source>This folder cannot be opened</source>
        <translation>Не удалось открыть эту папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="267"/>
        <source>No file matches &quot;%1&quot;</source>
        <translation>Нет файла, соответствующего &quot;%1&quot;</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="268"/>
        <source>This folder is empty</source>
        <translation>Эта папка пуста.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="342"/>
        <source>The file cannot be read.</source>
        <translation>Файл не может быть прочитан.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="352"/>
        <source>Unable to Open Folder</source>
        <translation>Не удалось открыть папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="353"/>
        <source>The folder cannot be opened. Please check permissions.</source>
        <translation>Папка не может быть открыта. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="411"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="639"/>
        <source>Move to Trash</source>
        <translation>Переместить в корзину</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="413"/>
        <source>Move &quot;%1&quot; to Trash?</source>
        <translation>Переместить &quot;%1&quot; в корзину?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="414"/>
        <source>Move %1 selected items to Trash?</source>
        <translation>Переместить выбранные элементы (%1) в корзину?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="436"/>
        <source>Unable to Move to Trash</source>
        <translation>Не удалось переместить в корзину</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="438"/>
        <source>The item &quot;%1&quot; could not be moved to Trash.</source>
        <translation>Элемент &quot;%1&quot; не удалось переместить в корзину.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="440"/>
        <source>%1 item(s) could not be moved to Trash.</source>
        <translation>Не удалось переместить в корзину элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="445"/>
        <source>Moved %1 item(s) to Trash.</source>
        <translation>Элементов перемещено в корзину: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="469"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="477"/>
        <source>Unable to Create Folder</source>
        <translation>Не удалось создать папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="470"/>
        <source>The target folder is not available.</source>
        <translation>Целевая папка недоступна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="478"/>
        <source>The folder could not be created. Please check permissions.</source>
        <translation>Не удалось создать папку. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="486"/>
        <source>Created folder: %1</source>
        <translation>Создана папка: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="507"/>
        <source>Unable to Create File</source>
        <translation>Не удалось создать файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="508"/>
        <source>The file could not be created. Please check permissions.</source>
        <translation>Не удалось создать файл. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="522"/>
        <source>Created file: %1</source>
        <translation>Создан файл: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="572"/>
        <source>New name:</source>
        <translation>Новое имя:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="581"/>
        <source>Invalid Name</source>
        <translation>Недопустимое имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="582"/>
        <source>The name must not contain path separators.</source>
        <translation>Имя не должно содержать разделителей пути.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="590"/>
        <source>Name Already Exists</source>
        <translation>Имя уже существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="591"/>
        <source>A file or folder with this name already exists.</source>
        <translation>Файл или папка с таким именем уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="596"/>
        <source>Unable to Rename</source>
        <translation>Не удалось переименовать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="597"/>
        <source>The file or folder could not be renamed.</source>
        <translation>Не удалось переименовать файл или папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="603"/>
        <source>Renamed &quot;%1&quot; to &quot;%2&quot;.</source>
        <translation>Переименовано из &quot;%1&quot; в &quot;%2&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="618"/>
        <source>Open the file in a GpgFrontend editor tab (Enter).</source>
        <translation>Открыть файл на вкладке редактора GpgFrontend (Enter).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="625"/>
        <source>Open with Default Application</source>
        <translation>Открыть в приложении по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="627"/>
        <source>Hand the item to the application your system uses for it.</source>
        <translation>Передать элемент приложению, которое ваша система использует для него.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="633"/>
        <source>Give the item a new name (F2).</source>
        <translation>Переименовать элемент (F2).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="641"/>
        <source>Move the selected items to the system Trash, where they can still be recovered (Delete).</source>
        <translation>Переместить выбранные элементы в системную корзину, откуда их можно восстановить (Delete).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="650"/>
        <source>Compute checksums of the file and show them on the information board, to compare it against a published value.</source>
        <translation>Вычислить контрольные суммы файла и показать их на информационной панели для сравнения с опубликованным значением.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="656"/>
        <source>Folder</source>
        <translation>Папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="658"/>
        <source>Create a new folder inside the selected folder.</source>
        <translation>Создать новую папку в выбранной папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="663"/>
        <source>Empty File</source>
        <translation>Пустой файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="665"/>
        <source>Create an empty file inside the selected folder.</source>
        <translation>Создать пустой файл в выбранной папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="675"/>
        <source>Copy Path</source>
        <translation>Копировать путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="677"/>
        <source>Copy the full path of the selected items to the clipboard.</source>
        <translation>Скопировать полный путь выбранных элементов в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="682"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="683"/>
        <source>Read this folder from disk again.</source>
        <translation>Перечитать эту папку с диска.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="687"/>
        <source>New</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="690"/>
        <source>Create a new item in the current folder.</source>
        <translation>Создать новый элемент в текущей папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="795"/>
        <source>Calculating</source>
        <translation>Вычисление</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="814"/>
        <source>File Hash Information</source>
        <translation>Информация о хеше файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="864"/>
        <source>Batch mode is enabled. Use Ctrl or Shift to select multiple items.</source>
        <translation>Включен пакетный режим. Используйте Ctrl или Shift для выбора нескольких элементов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1050"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1058"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1118"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1163"/>
        <source>Move Failed</source>
        <translation>Ошибка перемещения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1051"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1287"/>
        <source>A file or folder named &quot;%1&quot; already exists in the target folder.</source>
        <translation>Файл или папка с именем &quot;%1&quot; уже существует в целевой папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1059"/>
        <source>Unable to move &quot;%1&quot;.

The target may be on another volume, or you may not have sufficient permissions.</source>
        <translation>Не удалось переместить &quot;%1&quot;.

Возможно, целевой объект находится на другом томе или у вас недостаточно прав доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1118"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1173"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1286"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1300"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1309"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1324"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1332"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1357"/>
        <source>Copy Failed</source>
        <translation>Ошибка копирования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1119"/>
        <source>The target folder is not writable.</source>
        <translation>Целевая папка недоступна для записи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1142"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1148"/>
        <source>The source and target folder are the same.</source>
        <translation>Исходная и целевая папки совпадают.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1164"/>
        <source>Cannot move &quot;%1&quot; into itself or one of its subfolders.</source>
        <translation>Невозможно переместить &quot;%1&quot; в саму себя или одну из своих подпапок.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1174"/>
        <source>Cannot copy &quot;%1&quot; into itself or one of its subfolders.</source>
        <translation>Невозможно скопировать &quot;%1&quot; в саму себя или одну из своих подпапок.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1185"/>
        <source>Move Items</source>
        <translation>Перемещение элементов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1188"/>
        <source>Move &quot;%1&quot; to &quot;%2&quot;?</source>
        <translation>Переместить &quot;%1&quot; в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1190"/>
        <source>Move %1 items to &quot;%2&quot;?</source>
        <translation>Переместить %1 элементов в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1194"/>
        <source>Copy Items</source>
        <translation>Копирование элементов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1197"/>
        <source>Copy &quot;%1&quot; to &quot;%2&quot;?</source>
        <translation>Скопировать &quot;%1&quot; в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1199"/>
        <source>Copy %1 items to &quot;%2&quot;?</source>
        <translation>Скопировать %1 элементов в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1204"/>
        <source>Move</source>
        <translation>Переместить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1204"/>
        <source>Copy</source>
        <translation>Скопировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1246"/>
        <source>Moved %1 item(s).</source>
        <translation>Перемещено элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1247"/>
        <source>Copied %1 item(s).</source>
        <translation>Скопировано элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1259"/>
        <source>Move Partially Completed</source>
        <translation>Перемещение завершено частично</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1260"/>
        <source>Copy Partially Completed</source>
        <translation>Копирование завершено частично</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1262"/>
        <source>Some items could not be moved. Please check permissions or whether the target is on another volume.</source>
        <translation>Некоторые элементы не удалось переместить. Пожалуйста, проверьте права доступа или находится ли целевой объект на другом томе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1264"/>
        <source>Some items could not be copied. Please check permissions.</source>
        <translation>Некоторые элементы не удалось скопировать. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1301"/>
        <source>Unable to copy &quot;%1&quot;. Please check permissions.</source>
        <translation>Не удалось скопировать &quot;%1&quot;. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1310"/>
        <source>&quot;%1&quot; is not a regular file or folder.</source>
        <translation>&quot;%1&quot; не является обычным файлом или папкой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1325"/>
        <source>The target folder &quot;%1&quot; already exists.</source>
        <translation>Целевая папка &quot;%1&quot; уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1333"/>
        <source>Unable to create target folder &quot;%1&quot;.</source>
        <translation>Не удалось создать целевую папку &quot;%1&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1358"/>
        <source>Unable to copy &quot;%1&quot;.</source>
        <translation>Не удалось скопировать &quot;%1&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1378"/>
        <source>Path copied to clipboard.</source>
        <translation>Путь скопирован в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1379"/>
        <source>%1 paths copied to clipboard.</source>
        <translation>Пути (%1) скопированы в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1390"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1408"/>
        <source>File list refreshed.</source>
        <translation>Список файлов обновлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1398"/>
        <source>Unable to Refresh</source>
        <translation>Не удалось обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1399"/>
        <source>The current folder no longer exists or cannot be opened.</source>
        <translation>Текущая папка больше не существует или не может быть открыта.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FindWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/FindWidget.cpp" line="45"/>
        <source>Find</source>
        <translation>Найти</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GRTTreeView</name>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="103"/>
        <source>No runtime values published yet.</source>
        <translation>Значения времени выполнения еще не опубликованы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="119"/>
        <source>Copy Key Path</source>
        <translation>Копировать путь ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="125"/>
        <source>Copy Value</source>
        <translation>Копировать значение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="131"/>
        <source>Expand All</source>
        <translation>Развернуть все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/GRTTreeView.cpp" line="132"/>
        <source>Collapse All</source>
        <translation>Свернуть все</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GeneralTab</name>
    <message>
        <source>Clear gpg password cache when closing GpgFrontend.</source>
        <translation type="vanished">Очищать кэш паролей GPG при закрытии GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="51"/>
        <source>Base</source>
        <translation>Базовый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="58"/>
        <source>Module Loading Policy:</source>
        <translation>Политика загрузки модулей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="59"/>
        <source>Only Integrated Modules</source>
        <translation>Только встроенные модули</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="61"/>
        <source>All Modules</source>
        <translation>Все модули</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="62"/>
        <source>Disable</source>
        <translation>Отключить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="66"/>
        <source>Default Workspace As:</source>
        <translation>Рабочая область по умолчанию:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="67"/>
        <source>File Panel</source>
        <translation>Панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="68"/>
        <source>Text Editor</source>
        <translation>Текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="74"/>
        <source>File Panel opens at</source>
        <translation>Панель файлов открывается в</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="87"/>
        <source>Warn about keys expiring within:</source>
        <translation>Предупреждать об истечении ключей через:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="88"/>
        <source> days</source>
        <translation>дней</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="176"/>
        <source>Profile Workspace</source>
        <translation>Рабочая область профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="182"/>
        <source>Home Folder</source>
        <translation>Домашняя папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="185"/>
        <source>Working Directory</source>
        <translation>Рабочий каталог</translation>
    </message>
    <message>
        <source>Use home path as the default path for FilePanel</source>
        <translation type="vanished">Использовать домашний каталог как путь по умолчанию для панели файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="77"/>
        <source>Cache text editor contents.</source>
        <translation>Кэшировать содержимое текстового редактора.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="98"/>
        <source>System Default follows your operating system. Choose a language here to override it. GpgFrontend restarts automatically when you change it.</source>
        <translation>По умолчанию в системе следует за вашей операционной системой. Выберите здесь язык, чтобы переопределить его. GpgFrontend перезапустится автоматически при его изменении.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="64"/>
        <source>Operation</source>
        <translation>Операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="53"/>
        <source>Default Engine:</source>
        <translation>Движок по умолчанию:</translation>
    </message>
    <message>
        <source>rPGP Password Cache TTL (minutes):</source>
        <translation type="vanished">Срок хранения кэша паролей rPGP (мин):</translation>
    </message>
    <message>
        <source> min</source>
        <translation type="vanished">мин</translation>
    </message>
    <message>
        <source>Idle time the rPGP engine keeps an entered passphrase cached. The window is renewed each time the passphrase is used.</source>
        <translation type="vanished">Время бездействия, в течение которого движок rPGP хранит введённую парольную фразу в кэше. Период обновляется каждый раз при использовании парольной фразы.</translation>
    </message>
    <message>
        <source>rPGP Password Cache Max TTL (minutes):</source>
        <translation type="vanished">Максимальный срок хранения кэша паролей rPGP (мин):</translation>
    </message>
    <message>
        <source>Absolute lifetime of a cached passphrase, measured from when it was first entered, regardless of use. Never shorter than the TTL.</source>
        <translation type="vanished">Абсолютное время жизни кэшированной парольной фразы, отсчитываемое с момента первого ввода, независимо от использования. Никогда не короче TTL.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="80"/>
        <source>Import files dropped on the Key List without confirmation.</source>
        <translation>Импортировать файлы, перетащенные в список ключей, без подтверждения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="83"/>
        <source>Remember which keys are checked in the key list between restarts.</source>
        <translation>Запоминать, какие ключи отмечены в списке ключей между перезапусками.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="85"/>
        <source>Use Binary Mode for File Operations</source>
        <translation>Использовать бинарный режим для файловых операций</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="90"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="97"/>
        <source>NOTE</source>
        <translation>ПРИМЕЧАНИЕ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="97"/>
        <source>: </source>
        <translation>: </translation>
    </message>
    <message>
        <source>GpgFrontend will restart automatically if you change the language!</source>
        <translation type="vanished">GpgFrontend автоматически перезапустится при смене языка!</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="102"/>
        <source>Data</source>
        <translation>Данные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="104"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="141"/>
        <source>Clear All Data Objects (Total Size: %1)</source>
        <translation>Очистить все объекты данных (общий размер: %1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="120"/>
        <source>Reveal in File Explorer</source>
        <translation>Показать в проводнике</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="134"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="135"/>
        <source>Are you sure you want to clear all data objects?
This will result in loss of all cached form positions, statuses, key servers, etc.</source>
        <translation>Вы уверены, что хотите очистить все объекты данных?
Это приведёт к потере всех сохранённых позиций форм, статусов, серверов ключей и т.д.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GenerateCardKeyDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="52"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="53"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="54"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="55"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="56"/>
        <source>Non Expire</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="58"/>
        <source>Warning: This Operation will overwrite keys which exist on card!</source>
        <translation>Предупреждение: эта операция перезапишет ключи, находящиеся на карте!</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="59"/>
        <source>Generate</source>
        <translation>Сгенерировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="76"/>
        <source>Card Key Generation</source>
        <translation>Генерация ключей карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="95"/>
        <source>Name must not be empty.</source>
        <translation>Имя не должно быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="101"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Имя и комментарий не должны содержать символы &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; или управляющие символы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="107"/>
        <source>Please give a valid email address.</source>
        <translation>Укажите действительный адрес электронной почты.</translation>
    </message>
    <message>
        <source>Name must contain at least five characters.</source>
        <translation type="vanished">Имя должно содержать не менее пяти символов.</translation>
    </message>
    <message>
        <source>Please give a email address.</source>
        <translation type="vanished">Укажите адрес электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="150"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="151"/>
        <source>Card key generation completed successfully.</source>
        <translation>Генерация ключей карты успешно завершена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="157"/>
        <source>Generating</source>
        <translation>Генерация</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GenerateRevocationCert</name>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="44"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Без причины.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="44"/>
        <source>1 -&gt; This key is no more safe.</source>
        <translation>1 -&gt; Этот ключ больше не безопасен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="45"/>
        <source>2 -&gt; Key is outdated.</source>
        <translation>2 -&gt; Ключ устарел.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="45"/>
        <source>3 -&gt; Key is no longer used</source>
        <translation>3 -&gt; Ключ больше не используется</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="53"/>
        <source>Revocation Certificates</source>
        <translation>Сертификаты отзыва</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GenerateRevocationCert.cpp" line="65"/>
        <source>Generate revocation certificate</source>
        <translation>Создать сертификат отзыва</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GnuPGTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="47"/>
        <source>General</source>
        <translation>Общие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="48"/>
        <source>Advanced</source>
        <translation>Расширенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="50"/>
        <source>Enable GpgME Debug Log</source>
        <translation>Включить отладочный журнал GpgME</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="52"/>
        <source>Enable verbose GpgME logs for troubleshooting. This may include technical details about GnuPG operations.</source>
        <translation>Включить подробные журналы GpgME для устранения неполадок. Это может включать технические детали операций GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="55"/>
        <source>Use Custom GnuPG</source>
        <translation>Использовать пользовательский GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="56"/>
        <source>Select GnuPG Path</source>
        <translation>Выбрать путь к GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="58"/>
        <source>Terminate GnuPG background processes on exit</source>
        <translation>Завершать фоновые процессы GnuPG при выходе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="60"/>
        <source>This may affect other applications that are using GnuPG.</source>
        <translation>Это может повлиять на другие приложения, использующие GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="62"/>
        <source>Forbid all GnuPG network connection.</source>
        <translation>Запретить все сетевые соединения GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="67"/>
        <source>Clear password cache on exit</source>
        <translation>Очищать кеш паролей при выходе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="69"/>
        <source>Ask gpg-agent to forget all cached passphrases when GpgFrontend closes.</source>
        <translation>Запрашивать у gpg-agent забыть все кешированные парольные фразы при закрытии GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="75"/>
        <source>Select the directory that contains the &quot;gpgconf&quot; executable.</source>
        <translation>Выберите каталог, содержащий исполняемый файл «gpgconf».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="103"/>
        <source>Open Directory</source>
        <translation>Открыть каталог</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="157"/>
        <source>Maintenance</source>
        <translation>Обслуживание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="161"/>
        <source>These operations take effect immediately and are not undone by cancelling this dialog. Restarting components briefly interrupts any in-flight GnuPG operation.</source>
        <translation>Эти операции вступают в силу немедленно и не отменяются при отмене этого диалога. Перезапуск компонентов ненадолго прерывает любую выполняющуюся операцию GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="168"/>
        <source>Clear Password Cache</source>
        <translation>Очистить кэш паролей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="169"/>
        <source>Clear Password Cache of GnuPG</source>
        <translation>Очистить кэш паролей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="173"/>
        <source>Clear password cache successfully</source>
        <translation>Кэш паролей успешно очищен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="174"/>
        <source>Failed to clear password cache of GnuPG</source>
        <translation>Не удалось очистить кэш паролей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="178"/>
        <source>Reload Components</source>
        <translation>Перезагрузить компоненты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="179"/>
        <source>Reload All GnuPG&apos;s Components</source>
        <translation>Перезагрузить все компоненты GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="183"/>
        <source>Reload all the GnuPG&apos;s components successfully</source>
        <translation>Все компоненты GnuPG успешно перезагружены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="184"/>
        <source>Failed to reload all or one of the GnuPG&apos;s component(s)</source>
        <translation>Не удалось перезагрузить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="188"/>
        <source>Restart Components</source>
        <translation>Перезапустить компоненты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="189"/>
        <source>Restart All GnuPG&apos;s Components</source>
        <translation>Перезапустить все компоненты GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="193"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="194"/>
        <source>Are you sure you want to restart all of GnuPG&apos;s components?
Any GnuPG operation still running will be interrupted.</source>
        <translation>Вы уверены, что хотите перезапустить все компоненты GnuPG?
Любая выполняющаяся операция GnuPG будет прервана.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="201"/>
        <source>Restart all the GnuPG&apos;s components successfully</source>
        <translation>Все компоненты GnuPG успешно перезапущены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="202"/>
        <source>Failed to restart all or one of the GnuPG&apos;s component(s)</source>
        <translation>Не удалось перезапустить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="243"/>
        <source>Successful Operation</source>
        <translation>Операция выполнена успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="245"/>
        <source>Failed Operation</source>
        <translation>Неудавшаяся операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="343"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="350"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="364"/>
        <source>Illegal GnuPG Path</source>
        <translation>Недопустимый путь к GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="344"/>
        <source>Target GnuPG Path is not an exists readable directory.</source>
        <translation>Целевой путь к GnuPG не является существующим доступным для чтения каталогом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="351"/>
        <source>Target GnuPG Path is not an absolute path.</source>
        <translation>Целевой путь к GnuPG не является абсолютным путем.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="365"/>
        <source>Target GnuPG Path contains no &quot;gpgconf&quot; executable.</source>
        <translation>Целевой путь к GnuPG не содержит исполняемого файла «gpgconf».</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GpgFrontendApplication</name>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="74"/>
        <source>Standard Exception Thrown</source>
        <translation>Выброшено стандартное исключение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="75"/>
        <source>Oops, an standard exception was thrown during the running of the program. This is not a serious problem, it may be the negligence of the programmer, please report this problem if you can.</source>
        <translation>Произошло стандартное исключение во время работы программы. Это не серьёзная проблема, возможно это недосмотр разработчика. Пожалуйста, сообщите об этой проблеме, если можете.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="83"/>
        <source>Unhandled Exception Thrown</source>
        <translation>Выброшено необработанное исключение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="84"/>
        <source>Oops, an unhandled exception was thrown during the running of the program. This is not a serious problem, it may be the negligence of the programmer, please report this problem if you can.</source>
        <translation>Произошло необработанное исключение во время работы программы. Это не серьёзная проблема, возможно это недосмотр разработчика. Пожалуйста, сообщите об этой проблеме, если можете.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GpgOperaHelper</name>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="297"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="362"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="431"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="492"/>
        <source>Operation Not Supported</source>
        <translation>Операция не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="306"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="371"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="439"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="500"/>
        <source>Operation Cancelled</source>
        <translation>Операция отменена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="316"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="381"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="448"/>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="509"/>
        <source>Critical Error</source>
        <translation>Критическая ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="739"/>
        <source>Another operation is still running. Please wait for it to finish.</source>
        <translation>Другая операция всё ещё выполняется. Пожалуйста, дождитесь её завершения.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InfoBoardWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="70"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="71"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="72"/>
        <source>Magnify</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="73"/>
        <source>Clear</source>
        <translation>Очистить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="75"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="46"/>
        <source>Copy status text</source>
        <translation>Скопировать текст статуса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="76"/>
        <source>Save status text to file</source>
        <translation>Сохранить текст статуса в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="77"/>
        <source>Magnify the generated document</source>
        <translation>Увеличить масштаб сгенерированного документа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="78"/>
        <source>Clear status panel</source>
        <translation>Очистить панель состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="125"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="532"/>
        <source>Operation status will appear here.</source>
        <translation>Статус операции появится здесь.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="177"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="178"/>
        <source>Details</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="179"/>
        <source>Show the summary report</source>
        <translation>Показать сводный отчёт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="180"/>
        <source>Show the raw status text</source>
        <translation>Показать исходный текст статуса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="284"/>
        <source>The operation completed successfully.</source>
        <translation>Операция завершена успешно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="285"/>
        <source>%1 completed successfully.</source>
        <translation>%1 завершена успешно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="288"/>
        <source>Completed with warnings — please review the details.</source>
        <translation>Завершено с предупреждениями — пожалуйста, ознакомьтесь с деталями.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="289"/>
        <source>%1 completed with warnings — please review the details.</source>
        <translation>%1 завершено с предупреждениями — пожалуйста, ознакомьтесь с деталями.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="293"/>
        <source>The operation failed. See the details for more information.</source>
        <translation>Операция не выполнена. Подробности см. в деталях.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="295"/>
        <source>%1 failed. See the details for more information.</source>
        <translation>%1 не выполнена. Подробности см. в деталях.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="305"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="307"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="468"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="309"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="312"/>
        <source>Information</source>
        <translation>Сведения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="321"/>
        <source>The operation finished and everything checked out.</source>
        <translation>Операция завершена, всё проверено.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="323"/>
        <source>The operation finished, but something needs your attention. Read the details below.</source>
        <translation>Операция завершена, но требуется ваше внимание. Подробности см. ниже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="327"/>
        <source>The operation failed, or the result cannot be trusted. Read the details below.</source>
        <translation>Операция не выполнена, или результат не заслуживает доверия. Подробности см. ниже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="332"/>
        <source>Nothing has been checked yet. The light turns green, orange or red once an operation finishes.</source>
        <translation>Ещё ничего не проверено. Индикатор станет зелёным, оранжевым или красным после завершения операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="350"/>
        <source>Green</source>
        <translation>Зелёный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="350"/>
        <source>Everything went well</source>
        <translation>Всё прошло хорошо</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="351"/>
        <source>Orange</source>
        <translation>Оранжевый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="352"/>
        <source>Done, but check the details</source>
        <translation>Выполнено, но проверьте детали</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="353"/>
        <source>Red</source>
        <translation>Красный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="354"/>
        <source>Failed, or the result cannot be trusted</source>
        <translation>Не выполнено, или результат не заслуживает доверия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="355"/>
        <source>Grey</source>
        <translation>Серый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="356"/>
        <source>Idle, nothing checked yet</source>
        <translation>Ожидание, ничего не проверено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="384"/>
        <source>[%1] No details available.</source>
        <translation>[%1] Сведения отсутствуют.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="512"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="235"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="104"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="209"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="236"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="294"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="415"/>
        <source>DETAILS</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="46"/>
        <source>GPGFRONTEND SECURITY REPORT</source>
        <translation>ОТЧЁТ ПО БЕЗОПАСНОСТИ GPGFRONTEND</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="171"/>
        <source>No operation yet.
Results will appear here as a summary document.</source>
        <translation>Операций пока нет.
Результаты появятся здесь в виде сводного документа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="224"/>
        <source>OPERATION</source>
        <translation>ОПЕРАЦИЯ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="225"/>
        <source>STATUS</source>
        <translation>СТАТУС</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="226"/>
        <source>ENGINE</source>
        <translation>ДВИЖОК</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="44"/>
        <source>Copied</source>
        <translation>Скопировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="85"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="132"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="166"/>
        <source>Unable to Save</source>
        <translation>Не удалось сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="86"/>
        <source>The content could not be captured.</source>
        <translation>Не удалось скопировать содержимое.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="92"/>
        <source>GpgFrontend Security Report</source>
        <translation>Отчёт по безопасности GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="94"/>
        <source>Cryptographic operation result report</source>
        <translation>Отчёт о результатах криптографической операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="104"/>
        <source>Hash: </source>
        <translation>Хеш:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="133"/>
        <source>The image could not be saved.</source>
        <translation>Не удалось сохранить изображение.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="145"/>
        <source>Export Certificate</source>
        <translation>Экспортировать сертификат</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="146"/>
        <source>PNG Image (*.png);;All Files (*)</source>
        <translation>Изображения PNG (*.png);;Все файлы (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="159"/>
        <source>Save Status Panel Content</source>
        <translation>Сохранить содержимое панели состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="160"/>
        <source>Text Files (*.txt);;All Files (*)</source>
        <translation>Текстовые файлы (*.txt);;Все файлы (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="167"/>
        <source>The file could not be saved. Please check the path and permissions.</source>
        <translation>Не удалось сохранить файл. Проверьте путь и права доступа.</translation>
    </message>
    <message>
        <source>File Hash Information</source>
        <translation type="vanished">Информация о хеше файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="140"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="560"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="591"/>
        <source>Issued:  %1</source>
        <translation>Выдан:  %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="184"/>
        <source>Hash: %1</source>
        <translation>Хеш: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="189"/>
        <source>Export certificate as PNG image</source>
        <translation>Экспортировать сертификат в формате PNG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="202"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="342"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="570"/>
        <source>Operation: %1</source>
        <translation>Операция: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="204"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="344"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="572"/>
        <source>Status:    %1</source>
        <translation>Статус:    %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="205"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="347"/>
        <source>SHA-256:   %1</source>
        <translation>SHA-256:   %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="230"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="410"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="634"/>
        <source>Decrypt</source>
        <translation>Расшифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="231"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="411"/>
        <source>RECIPIENT</source>
        <translation>ПОЛУЧАТЕЛЬ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="232"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="412"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="233"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="412"/>
        <source>Verify</source>
        <translation>Проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="234"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="413"/>
        <source>SIGNER</source>
        <translation>ПОДПИСАНТ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="345"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="573"/>
        <source>Engine:    %1</source>
        <translation>Движок:    %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="351"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="577"/>
        <source>Details:   %1</source>
        <translation>Детали:    %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="465"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="471"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="502"/>
        <source>… and %1 more</source>
        <translation>… и ещё %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="582"/>
        <source>  SHA-256: %1</source>
        <translation>SHA-256: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="619"/>
        <source>  Signer: %1</source>
        <translation>Подписант: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="621"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="628"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="638"/>
        <source>  Key ID: %1</source>
        <translation>Идентификатор ключа: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="626"/>
        <source>  Signed: %1</source>
        <translation>Подписано: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="631"/>
        <source>  Invalid signer: %1 — %2</source>
        <translation>Недействительный подписант: %1 — %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="633"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="637"/>
        <source>  Recipient: %1</source>
        <translation>Получатель: %1</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InfoTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="351"/>
        <source>Developer</source>
        <translation>Разработчик</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="327"/>
        <source>A user-friendly OpenPGP tool for encryption, signing, and key management.</source>
        <translation>Удобный инструмент OpenPGP для шифрования, подписи и управления ключами.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="346"/>
        <source>Developed and maintained by Saturneric.</source>
        <translation>Разработано и поддерживается Saturneric.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="347"/>
        <source>Report an issue on GitHub</source>
        <translation>Сообщить о проблеме на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="348"/>
        <source>About and contact information</source>
        <translation>О программе и контакты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="373"/>
        <source>Website:</source>
        <translation>Веб-сайт:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="377"/>
        <source>Documentation:</source>
        <translation>Документация:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="380"/>
        <source>User guides and overview</source>
        <translation>Руководства пользователя и обзор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="382"/>
        <source>Source code:</source>
        <translation>Исходный код:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="386"/>
        <source>Release notes:</source>
        <translation>Примечания к выпуску:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="389"/>
        <source>Changelog and downloads</source>
        <translation>Список изменений и загрузки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="392"/>
        <source>Resources</source>
        <translation>Ресурсы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="399"/>
        <source>GpgFrontend is free software, licensed under &lt;a href=&quot;https://www.gnu.org/licenses/gpl-3.0.html&quot;&gt;GPL-3.0-or-later&lt;/a&gt;.</source>
        <translation>GpgFrontend — это свободное программное обеспечение, распространяемое под лицензией &lt;a href=&quot;https://www.gnu.org/licenses/gpl-3.0.html&quot;&gt;GPL-3.0-or-later&lt;/a&gt;.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InstantMessagingTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="69"/>
        <source>Message Book Phrase</source>
        <translation>Фраза из книги сообщений</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="73"/>
        <source>A long secret you share with one friend. It makes your messages look like random text, so nobody can tell they are PGP at all. You and your friend must use exactly the same phrase.</source>
        <translation>Длинный секрет, которым вы делитесь с одним другом. Он делает ваши сообщения похожими на случайный текст, так что никто не сможет понять, что это PGP. Вы и ваш друг должны использовать одну и ту же фразу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="86"/>
        <source>No phrase set. Messages use the built-in default book.</source>
        <translation>Фраза не задана. Сообщения используют встроенную книгу по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="102"/>
        <source>Generate</source>
        <translation>Сгенерировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="104"/>
        <source>Create a new random phrase. Share it with your friend so you both use the same one.</source>
        <translation>Создать новую случайную фразу. Поделитесь ею с другом, чтобы вы оба использовали одну и ту же.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="114"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="219"/>
        <source>Show</source>
        <translation>Показать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="115"/>
        <source>Show or hide the phrase.</source>
        <translation>Показать или скрыть фразу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="119"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="170"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="120"/>
        <source>Copy the phrase to the clipboard.</source>
        <translation>Скопировать фразу в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="124"/>
        <source>Paste</source>
        <translation>Вставить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="126"/>
        <source>Replace the phrase with the one on the clipboard.</source>
        <translation>Заменить фразу на ту, что в буфере обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="132"/>
        <source>Clear</source>
        <translation>Очистить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="134"/>
        <source>Remove the phrase and fall back to the default book.</source>
        <translation>Удалить фразу и вернуться к книге по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="152"/>
        <source>Book Fingerprint</source>
        <translation>Отпечаток книги</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="156"/>
        <source>A short code made from your phrase. Read it out with your friend to be sure you both have the same one. Unlike the phrase, this code is safe to say out loud.</source>
        <translation>Короткий код, созданный из вашей фразы. Прочитайте его вслух вместе с другом, чтобы убедиться, что у вас обоих одна и та же фраза. В отличие от фразы, этот код можно произносить вслух.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="171"/>
        <source>Copy the fingerprint to the clipboard.</source>
        <translation>Скопировать отпечаток в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="183"/>
        <source>The phrase is stored in the encrypted cache, never in the settings file. Send it to your friend over a private channel.</source>
        <translation>Фраза хранится в зашифрованном кеше, а не в файле настроек. Отправьте её другу по приватному каналу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="219"/>
        <source>Hide</source>
        <translation>Скрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="239"/>
        <source>No phrase set. Using the built-in default.</source>
        <translation>Фраза не задана. Используется встроенная по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="240"/>
        <source>Phrase set. %1 characters.</source>
        <translation>Фраза задана. %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsIM.cpp" line="250"/>
        <source>Calculating…</source>
        <translation>Вычисление…</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::IntroPage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="318"/>
        <source>Welcome to GpgFrontend</source>
        <translation>Добро пожаловать в GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="319"/>
        <source>Let&apos;s get you started in just a moment.</source>
        <translation>Совсем скоро мы поможем вам начать работу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="322"/>
        <source>A simple, privacy-focused OpenPGP tool for text, files, and keys.</source>
        <translation>Простой инструмент OpenPGP, ориентированный на конфиденциальность, для текста, файлов и ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="325"/>
        <source>Language:</source>
        <translation>Язык:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="326"/>
        <source>System Default</source>
        <translation>По умолчанию в системе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="329"/>
        <source>&lt;b&gt;GpgFrontend&lt;/b&gt; helps you encrypt, decrypt, sign, and verify messages and files with OpenPGP. This short wizard will point you to the most useful places to start.</source>
        <translation>&lt;b&gt;GpgFrontend&lt;/b&gt; помогает шифровать, расшифровывать, подписывать и проверять сообщения и файлы с помощью OpenPGP. Этот краткий мастер поможет вам начать работу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="348"/>
        <source>You can change update checking, key database, and appearance settings later from the application settings.</source>
        <translation>Вы можете изменить настройки проверки обновлений, базу данных ключей и внешний вид позже в настройках приложения.</translation>
    </message>
    <message>
        <source>You can change language, update checking, key database, and appearance settings later from the application settings.</source>
        <translation type="vanished">Вы можете изменить язык, настройки проверки обновлений, базу данных ключей и внешний вид позже в настройках приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="334"/>
        <source>Star GpgFrontend on GitHub</source>
        <translation>Поставить звезду GpgFrontend на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="335"/>
        <source>GpgFrontend is free and open source. A star helps more people discover it and keeps the project moving forward.</source>
        <translation>GpgFrontend — бесплатное приложение с открытым исходным кодом. Звезда помогает большему количеству людей узнать о нём и способствует развитию проекта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="339"/>
        <source>Open the overview page</source>
        <translation>Открыть страницу обзора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="340"/>
        <source>Get a quick tour of the main features and common workflows.</source>
        <translation>Краткий обзор основных функций и типичных рабочих процессов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="343"/>
        <source>Fundamental concepts</source>
        <translation>Основные понятия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="344"/>
        <source>Understand public keys, private keys, encryption, signing, and trust.</source>
        <translation>Понимание открытых ключей, закрытых ключей, шифрования, подписи и доверия.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyActionState</name>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="44"/>
        <source>Select a key first.</source>
        <translation>Сначала выберите ключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="49"/>
        <source>Select exactly one key for this.</source>
        <translation>Выберите ровно один ключ для этого.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="54"/>
        <source>Only available for a key, not a key group.</source>
        <translation>Доступно только для ключа, а не для группы ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="60"/>
        <source>Only available for a key you own the private half of.</source>
        <translation>Доступно только для ключа, для которого у вас есть закрытый ключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="66"/>
        <source>Check or select at least one key first.</source>
        <translation>Сначала отметьте или выберите хотя бы один ключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="139"/>
        <source>This key carries no email address.</source>
        <translation>У этого ключа нет адреса электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="176"/>
        <source>Certifying is for vouching for someone else&apos;s key.</source>
        <translation>Сертификация предназначена для подтверждения чужого ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="198"/>
        <source>Tick the box beside at least one key first.</source>
        <translation>Сначала установите флажок рядом хотя бы с одним ключом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="234"/>
        <source>Expiry can only be changed on keys you own.</source>
        <translation>Срок действия можно изменить только для ключей, которыми вы владеете.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="245"/>
        <source>This keyring holds no private keys.</source>
        <translation>Эта связка ключей не содержит закрытых ключей.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDatabaseEditDialog</name>
    <message>
        <source>Key Database Name</source>
        <translation type="vanished">Имя базы данных ключей</translation>
    </message>
    <message>
        <source>Key Database Path</source>
        <translation type="vanished">Путь к базе данных ключей</translation>
    </message>
    <message>
        <source>Key Database Engine</source>
        <translation type="vanished">Движок базы данных ключей</translation>
    </message>
    <message>
        <source>Select A Key Database Path</source>
        <translation type="vanished">Выберите путь к базе данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="117"/>
        <source>Convert to Relative Path</source>
        <translation>Преобразовать в относительный путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="123"/>
        <source>Key Database Info</source>
        <translation>Информация о базе данных ключей</translation>
    </message>
    <message>
        <source>Key Database Path (Automatically Generated)</source>
        <translation type="vanished">Путь к базе данных ключей (автоматически сгенерирован)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="112"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="113"/>
        <source>Path</source>
        <translation>Путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="114"/>
        <source>Engine</source>
        <translation>Движок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="115"/>
        <source>Mode</source>
        <translation>Режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="116"/>
        <source>Choose Folder…</source>
        <translation>Выберите папку…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="118"/>
        <source>Automatic</source>
        <translation>Автоматический</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="119"/>
        <source>Manual</source>
        <translation>Ручной</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="120"/>
        <source>e.g. Personal Keys</source>
        <translation>например, Личные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="132"/>
        <source>Open Directory</source>
        <translation>Открыть каталог</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="140"/>
        <source>Illegal GnuPG Key Database Path</source>
        <translation>Недопустимый путь к базе данных ключей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="141"/>
        <source>Target GnuPG Key Database Path is not an exists readable directory.</source>
        <translation>Целевой путь к базе данных ключей GnuPG не является существующим доступным для чтения каталогом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="205"/>
        <source>GpgFrontend picks a safe location for you. Just choose a name.</source>
        <translation>GpgFrontend выберет безопасное расположение за вас. Просто укажите имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="209"/>
        <source>Pick an existing folder yourself. For advanced setups.</source>
        <translation>Выберите существующую папку самостоятельно. Для расширенных настроек.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="224"/>
        <source>The key database name cannot be empty.</source>
        <translation>Имя базы данных ключей не может быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="229"/>
        <source>The key database path cannot be empty.</source>
        <translation>Путь к базе данных ключей не может быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="235"/>
        <source>A key database with the name &apos;%1&apos; already exists. Please choose a different name.</source>
        <translation>База данных ключей с именем «%1» уже существует. Выберите другое имя.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDatabasesTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="78"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="78"/>
        <source>Backend Type</source>
        <translation>Тип бэкенда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="78"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="79"/>
        <source>Path</source>
        <translation>Путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="79"/>
        <source>Real Path</source>
        <translation>Реальный путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="129"/>
        <source>Add New Key Database</source>
        <translation>Добавить новую базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="180"/>
        <source>Unreadable Key Database Settings</source>
        <translation>Нечитаемые настройки базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="181"/>
        <source>Your saved key database list exists but could not be read. This usually means it was written by another installation, or with a different application key.</source>
        <translation>Ваш сохранённый список баз данных ключей существует, но не может быть прочитан. Обычно это означает, что он был записан другой установкой или с другим ключом приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="185"/>
        <source>Saving now replaces it with the list shown here. The previous list cannot be recovered afterwards.</source>
        <translation>Сохранение сейчас заменит его на список, показанный здесь. Предыдущий список не может быть восстановлен впоследствии.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="247"/>
        <source>N/A</source>
        <translation>Н/Д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="343"/>
        <source>No Key Database Selected</source>
        <translation>База данных ключей не выбрана</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="344"/>
        <source>Please select a key database to edit.</source>
        <translation>Пожалуйста, выберите базу данных ключей для редактирования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="363"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="416"/>
        <source>Duplicate Key Database Paths</source>
        <translation>Дублирующиеся пути к базам данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="364"/>
        <source>The edited key database path duplicates a previously existing one.</source>
        <translation>Указанный путь к базе данных ключей дублирует уже существующий.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="374"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="449"/>
        <source>Invalid Key Database Paths</source>
        <translation>Недопустимые пути к базам данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="375"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="450"/>
        <source>The edited key database path is not a valid path that GpgFrontend can use</source>
        <translation>Указанный путь к базе данных ключей не является допустимым путем, который может использовать GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="400"/>
        <source>Maximum Key Database Limit Reached</source>
        <translation>Достигнут лимит баз данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="401"/>
        <source>Currently, GpgFrontend supports a maximum of 8 key databases. Please remove an existing database to add a new one.</source>
        <translation>В настоящее время GpgFrontend поддерживает максимум 8 баз данных ключей. Пожалуйста, удалите существующую базу данных, чтобы добавить новую.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="417"/>
        <source>The newly added key database path duplicates a previously existing one.</source>
        <translation>Добавленный путь к базе данных ключей дублирует уже существующий.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="426"/>
        <source>Invalid Key Database Path</source>
        <translation>Недопустимый путь к базе данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="427"/>
        <source>The specified key database path points to an existing file. Please specify a path that does not exist or points to a directory.</source>
        <translation>Указанный путь к базе данных ключей указывает на существующий файл. Пожалуйста, укажите путь, который не существует или указывает на каталог.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="438"/>
        <source>Failed to Create Key Database Directory</source>
        <translation>Не удалось создать каталог базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="439"/>
        <source>GpgFrontend failed to create a directory at the specified key database path. Please check the path and your permissions.</source>
        <translation>GpgFrontend не удалось создать каталог по указанному пути к базе данных ключей. Пожалуйста, проверьте путь и ваши права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="484"/>
        <source>Confirm Deletion</source>
        <translation>Подтверждение удаления</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="485"/>
        <source>Are you sure you want to remove the selected key database from the list?</source>
        <translation>Вы уверены, что хотите удалить выбранную базу данных ключей из списка?</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDetailsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="53"/>
        <source>KeyPair</source>
        <translation>Ключевая пара</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="66"/>
        <source>UIDs</source>
        <translation>Идентификаторы пользователя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="72"/>
        <source>Photo IDs</source>
        <translation>Фото-идентификаторы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="77"/>
        <source>Keychain</source>
        <translation>Связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="80"/>
        <source>Operations</source>
        <translation>Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="98"/>
        <source>Key Details</source>
        <translation>Сведения о ключе</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGenerateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1394"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1489"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1495"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1592"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1761"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1799"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="167"/>
        <source>Custom</source>
        <translation>Пользовательский</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="173"/>
        <source>3 Months</source>
        <translation>3 месяца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="176"/>
        <source>6 Months</source>
        <translation>6 месяцев</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="179"/>
        <source>1 Year</source>
        <translation>1 год</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="163"/>
        <source>2 Years</source>
        <translation>2 года</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="183"/>
        <source>5 Years</source>
        <translation>5 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="186"/>
        <source>10 Years</source>
        <translation>10 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="528"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="539"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="189"/>
        <source>Non Expired</source>
        <translation>Действующий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="483"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="878"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1500"/>
        <source>Primary Key Only</source>
        <translation>Только первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="484"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="941"/>
        <source>Primary Key With Subkey</source>
        <translation>Первичный ключ с подключом</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="490"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="499"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="491"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="492"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="498"/>
        <source>Key Database</source>
        <translation>База данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="512"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="530"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="500"/>
        <source>Combination</source>
        <translation>Комбинация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="487"/>
        <source>Profile</source>
        <translation>Профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="501"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="513"/>
        <source>Validity Period</source>
        <translation>Срок действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1747"/>
        <source>Save Profile</source>
        <translation>Сохранить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="505"/>
        <source>Save current configuration as a new profile</source>
        <translation>Сохранить текущую конфигурацию как новый профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1818"/>
        <source>Delete Profile</source>
        <translation>Удалить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="507"/>
        <source>Delete current selected profile</source>
        <translation>Удалить текущий выбранный профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="488"/>
        <source>Basic</source>
        <translation>Основные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="503"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="506"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="564"/>
        <source>%1 Engine Version: %2</source>
        <translation>Версия движка %1: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="722"/>
        <source>Name must not be empty.</source>
        <translation>Имя не должно быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1845"/>
        <source>Reset To Default</source>
        <translation>Сбросить по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="510"/>
        <source>Reset profile list to default configuration</source>
        <translation>Сбросить список профилей к настройкам по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="496"/>
        <source>Fill in a random anonymous identity (for throwaway or test keys)</source>
        <translation>Заполнить случайный анонимный идентификатор (для одноразовых или тестовых ключей)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="508"/>
        <source>Reset</source>
        <translation>Сбросить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="514"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="532"/>
        <source>Key Length</source>
        <translation>Длина ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="515"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="534"/>
        <source>Second Algorithm</source>
        <translation>Второй алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="516"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="535"/>
        <source>Second Key Length</source>
        <translation>Длина второго ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="517"/>
        <source>Key Format</source>
        <translation>Формат ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="518"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="533"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="522"/>
        <source>v4 (Compatible)</source>
        <translation>v4 (Совместимый)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="523"/>
        <source>v6 (Modern)</source>
        <translation>v6 (Современный)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="524"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="536"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="525"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="537"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="526"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="538"/>
        <source>Authentication</source>
        <translation>Аутентификация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="527"/>
        <source>No Passphrase</source>
        <translation>Без парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="531"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="542"/>
        <source>Easy Mode</source>
        <translation>Простой режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="543"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="544"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="547"/>
        <source>Generate</source>
        <translation>Сгенерировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="569"/>
        <source>If subkey is specified, it will be generated together with the primary key. Therefore, you may need to enter the passphrase additionally for the subkey generation.</source>
        <translation>Если указан подключ, он будет сгенерирован вместе с первичным ключом. Поэтому может потребоваться дополнительный ввод парольной фразы для генерации подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="575"/>
        <source>Supported Primary Key Algorithms: </source>
        <translation>Поддерживаемые алгоритмы первичного ключа: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="585"/>
        <source>Supported Subkey Algorithms: </source>
        <translation>Поддерживаемые алгоритмы подключа: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="595"/>
        <source>Please select a key algorithm and configure the parameters as needed.</source>
        <translation>Выберите алгоритм ключа и настройте параметры.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="605"/>
        <source>Generate Key</source>
        <translation>Сгенерировать ключ</translation>
    </message>
    <message>
        <source>Name must contain at least five characters.</source>
        <translation type="vanished">Имя должно содержать не менее пяти символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="728"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Имя и комментарий не должны содержать символы &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; или управляющие символы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="734"/>
        <source>Please give a valid email address.</source>
        <translation>Укажите корректный адрес электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="739"/>
        <source>Please give a valid primary key algorithm.</source>
        <translation>Укажите допустимый алгоритм первичного ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="748"/>
        <source>Time to primary key expiration must not be less than 120 seconds.</source>
        <translation>Время до истечения первичного ключа должно быть не менее 120 секунд.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="755"/>
        <source>Please give a valid subkey algorithm.</source>
        <translation>Укажите допустимый алгоритм подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="763"/>
        <source>Time to subkey expiration must not be less than 120 seconds.</source>
        <translation>Время до истечения подключа должно быть не менее 120 секунд.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="793"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="794"/>
        <source>Key generation completed successfully.</source>
        <translation>Генерация ключа успешно завершена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1525"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1526"/>
        <source>Unknown error occurred</source>
        <translation>Произошла неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1541"/>
        <source>Generating</source>
        <translation>Генерация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1669"/>
        <source>ECC</source>
        <translation>ECC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1674"/>
        <source>Post-Quantum</source>
        <translation>Постквантовый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1748"/>
        <source>Please enter profile name:</source>
        <translation>Введите имя профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1756"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1762"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1770"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1784"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1800"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1812"/>
        <source>Notice</source>
        <translation>Уведомление</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1757"/>
        <source>Profile was not saved: Name cannot be empty.</source>
        <translation>Профиль не сохранён: имя не может быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1763"/>
        <source>The profile name &apos;Custom&apos; is reserved. Please choose another name.</source>
        <translation>Имя профиля «Custom» зарезервировано. Выберите другое имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1771"/>
        <source>Profile was not saved: Name cannot be longer than %1 characters.</source>
        <translation>Профиль не сохранён: имя не может быть длиннее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1785"/>
        <source>Profile was not saved: Name already exists.</source>
        <translation>Профиль не сохранён: имя уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1801"/>
        <source>The &apos;Custom&apos; profile cannot be deleted.</source>
        <translation>Профиль «Custom» нельзя удалить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1813"/>
        <source>Selected profile does not exist.</source>
        <translation>Выбранный профиль не существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1819"/>
        <source>Are you sure you want to delete the profile &apos;%1&apos;?</source>
        <translation>Вы уверены, что хотите удалить профиль «%1»?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1846"/>
        <source>Are you sure you want to reset the easy profile configuration to default? This action cannot be undone.</source>
        <translation>Вы уверены, что хотите сбросить конфигурацию простого профиля по умолчанию? Это действие нельзя отменить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="2013"/>
        <source>Post-quantum algorithms require the v6 key format.</source>
        <translation>Постквантовые алгоритмы требуют формат ключа v6.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupCreationDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="50"/>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="55"/>
        <source>A Key Group is a collection of keys. It allows you to encrypt data for multiple recipients at once by grouping their public keys together.</source>
        <translation>Группа ключей — это набор ключей. Она позволяет шифровать данные для нескольких получателей одновременно, объединяя их открытые ключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="63"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="64"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="65"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="84"/>
        <source>New Key Group</source>
        <translation>Новая группа ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="97"/>
        <source>Name must contain at least five characters.</source>
        <translation>Имя должно содержать не менее пяти символов.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupManageDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="60"/>
        <source>Key Group</source>
        <translation>Группа ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="74"/>
        <source>Default</source>
        <translation>По умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="91"/>
        <source>Key(s) in Key Group: </source>
        <translation>Ключ(и) в группе ключей: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="92"/>
        <source>Key(s) in Key Database: </source>
        <translation>Ключ(и) в базе данных ключей: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="97"/>
        <source>Key Group Management</source>
        <translation>Управление группами ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="137"/>
        <source>Some Keys Failed</source>
        <translation>Не удалось обработать некоторые ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="138"/>
        <source>Some keys could not be added to the group:
%1</source>
        <translation>Некоторые ключи не удалось добавить в группу:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="170"/>
        <source>This Key Group contains some invalid keys:

%1

These keys are no longer available. Do you want to remove them from the group?</source>
        <translation>Эта группа ключей содержит недопустимые ключи:

%1

Эти ключи больше недоступны. Хотите удалить их из группы?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="176"/>
        <source>Invalid Keys in Group</source>
        <translation>Недопустимые ключи в группе</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyImportDetailDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="44"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="65"/>
        <source>Key Import Details</source>
        <translation>Подробности импорта ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="45"/>
        <source>No keys found to import</source>
        <translation>Не найдено ключей для импорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="162"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="176"/>
        <source>Considered</source>
        <translation>Рассмотрено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="163"/>
        <source>Imported</source>
        <translation>Импортировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="165"/>
        <source>Not Imported</source>
        <translation>Не импортировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="169"/>
        <source>New Revocations</source>
        <translation>Новые отзывы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="215"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="215"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="215"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="216"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="260"/>
        <source>Private</source>
        <translation>Закрытый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="263"/>
        <source>Public</source>
        <translation>Открытый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="164"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="266"/>
        <source>Unchanged</source>
        <translation>Без изменений</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="104"/>
        <source>Import Completed with Issues</source>
        <translation>Импорт завершён с ошибками</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="105"/>
        <source>%1 of %2 key(s) could not be imported.</source>
        <translation>%1 из %2 ключей не удалось импортировать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="110"/>
        <source>Import Successful</source>
        <translation>Импорт выполнен успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="112"/>
        <source>Successfully processed %1 key(s).</source>
        <translation>Успешно обработано %1 ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="114"/>
        <source>Nothing to Import</source>
        <translation>Нечего импортировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="115"/>
        <source>All %1 key(s) are already up to date.</source>
        <translation>Все %1 ключей уже актуальны.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="144"/>
        <source>Summary</source>
        <translation>Сводка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="166"/>
        <source>Private Keys Read</source>
        <translation>Прочитано закрытых ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="167"/>
        <source>Private Keys Imported</source>
        <translation>Импортировано закрытых ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="168"/>
        <source>Private Keys Unchanged</source>
        <translation>Закрытых ключей без изменений</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="269"/>
        <source>New Key</source>
        <translation>Новый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="272"/>
        <source>New Subkey</source>
        <translation>Новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="276"/>
        <source>New Signature</source>
        <translation>Новая подпись</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="280"/>
        <source>New UID</source>
        <translation>Новый UID</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyList</name>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="552"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="554"/>
        <source>Refresh the key list to synchronize changes.</source>
        <translation>Обновите список ключей для синхронизации изменений.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="556"/>
        <source>Sync Public Key</source>
        <translation>Синхронизировать открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="666"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="676"/>
        <source>Subkeys</source>
        <translation>Подключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="684"/>
        <source>No optional columns</source>
        <translation>Нет дополнительных столбцов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1363"/>
        <source>Refreshing Key List...</source>
        <translation>Обновление списка ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1443"/>
        <source>Import Keys</source>
        <translation>Импортировать ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1368"/>
        <source>Key List Refreshed.</source>
        <translation>Список ключей обновлён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="668"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <source>Sync public keys with the default keyserver.</source>
        <translation type="vanished">Синхронизировать открытые ключи с сервером ключей по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="304"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1232"/>
        <source>New Category...</source>
        <translation>Новая категория...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="558"/>
        <source>Sync public keys with the key server configured as the default.</source>
        <translation>Синхронизировать открытые ключи с сервером ключей, заданным по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="560"/>
        <source>Uncheck All</source>
        <translation>Снять выделение со всех</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="561"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="847"/>
        <source>Uncheck all keys in the current tab.</source>
        <translation>Снять выделение со всех ключей на текущей вкладке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="563"/>
        <source>Check All</source>
        <translation>Выделить все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="564"/>
        <source>Check all keys in the current tab.</source>
        <translation>Выделить все ключи на текущей вкладке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="567"/>
        <source>Search keys by user ID, key ID, fingerprint...</source>
        <translation>Поиск ключей по идентификатору пользователя, идентификатору ключа, отпечатку...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="569"/>
        <source>Columns</source>
        <translation>Столбцы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="570"/>
        <source>Choose visible key table columns.</source>
        <translation>Выберите видимые столбцы таблицы ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="574"/>
        <source>Create a key group from checked encryption-capable keys.</source>
        <translation>Создать группу ключей из выбранных ключей с возможностью шифрования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="578"/>
        <source>Switch between key databases.</source>
        <translation>Переключение между базами данных ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="601"/>
        <source>Channel %1</source>
        <translation>Канал %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="630"/>
        <source>No key database available</source>
        <translation>Базы данных ключей недоступны</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="638"/>
        <source>Key Database</source>
        <translation>База данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="672"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="674"/>
        <source>Owner Trust</source>
        <translation>Доверие владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="680"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="689"/>
        <source>Reset Column Widths</source>
        <translation>Сбросить ширины столбцов</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="849"/>
        <source>%n key(s) checked</source>
        <translation>
            <numerusform>%n ключ(ей) выбрано</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="857"/>
        <source>%n of them are not shown by the current category or search filter.</source>
        <translation>
            <numerusform>%n из них не отображаются из-за текущей категории или фильтра поиска.</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1171"/>
        <source>Delete Category</source>
        <translation>Удалить категорию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1172"/>
        <source>Delete category &quot;%1&quot;? This removes the grouping only; the keys themselves are not affected.</source>
        <translation>Удалить категорию &quot;%1&quot;? Это удаляет только группировку; сами ключи не затрагиваются.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1238"/>
        <source>Set Colour...</source>
        <translation>Установить цвет...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1242"/>
        <source>Reset Colour</source>
        <translation>Сбросить цвет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1253"/>
        <source>Rename Category...</source>
        <translation>Переименовать категорию...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1257"/>
        <source>Delete Category...</source>
        <translation>Удалить категорию...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1271"/>
        <source>Choose Category Colour</source>
        <translation>Выбрать цвет категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1307"/>
        <source>New Category</source>
        <translation>Новая категория</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1307"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1326"/>
        <source>Category name:</source>
        <translation>Имя категории:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1313"/>
        <source>Category Colour (optional)</source>
        <translation>Цвет категории (необязательно)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1326"/>
        <source>Rename Category</source>
        <translation>Переименовать категорию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1445"/>
        <source>You&apos;ve dropped something on the key list.
GpgFrontend will now try to import key(s).</source>
        <translation>Вы перетащили объект в список ключей.
GpgFrontend попытается импортировать ключ(и).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1450"/>
        <source>Ask before importing keys next time.</source>
        <translation>Спрашивать перед импортом ключей в следующий раз.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1713"/>
        <source>No key matches &quot;%1&quot;</source>
        <translation>Ни один ключ не соответствует &quot;%1&quot;</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1714"/>
        <source>%n key(s) match &quot;%1&quot;</source>
        <translation>
            <numerusform>%n ключ(ей) соответствует &quot;%1&quot;</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="670"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="678"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="572"/>
        <source>New Key Group</source>
        <translation>Новая группа ключей</translation>
    </message>
    <message>
        <source>Key Databases</source>
        <translation type="vanished">Базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1601"/>
        <source>The key has been updated</source>
        <translation>Ключ обновлён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1603"/>
        <source>No need to update the key</source>
        <translation>Обновление ключа не требуется</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1639"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1667"/>
        <source>Syncing Key List...</source>
        <translation>Синхронизация списка ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1644"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1672"/>
        <source>Sync [%1/%2] %3 %4</source>
        <translation>Синхронизация [%1/%2] %3 %4</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1654"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1682"/>
        <source>Key List Sync Done.</source>
        <translation>Синхронизация списка ключей завершена.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyMgmt</name>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="79"/>
        <source>All</source>
        <translation>Все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="84"/>
        <source>Key Group</source>
        <translation>Группа ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="90"/>
        <source>Only Public Key</source>
        <translation>Только открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="98"/>
        <source>Has Private Key</source>
        <translation>Имеет закрытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="106"/>
        <source>Expiring Soon</source>
        <translation>Скоро истекает</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="112"/>
        <source>No Primary Key</source>
        <translation>Нет первичного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="122"/>
        <source>Revoked</source>
        <translation>Отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="129"/>
        <source>Expired</source>
        <translation>Срок истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="136"/>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="185"/>
        <source>KeyPair Management</source>
        <translation>Управление ключевыми парами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="242"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="252"/>
        <source>Export</source>
        <translation>Экспортировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="260"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="751"/>
        <source>Key Operations</source>
        <translation>Операции с ключами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="271"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="709"/>
        <source>Keyserver</source>
        <translation>Сервер ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="277"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1603"/>
        <source>Category</source>
        <translation>Категория</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="338"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="338"/>
        <source>Open Key File</source>
        <translation>Открыть файл ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="345"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="350"/>
        <source>New Keypair</source>
        <translation>Новая пара ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="351"/>
        <source>Generate KeyPair</source>
        <translation>Сгенерировать пару ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="358"/>
        <source>New Subkey</source>
        <translation>Новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="359"/>
        <source>Generate Subkey For Selected KeyPair</source>
        <translation>Сгенерировать подключ для выбранной пары ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="368"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="648"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="369"/>
        <source>Import New Key From File</source>
        <translation>Импортировать новый ключ из файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="376"/>
        <source>Clipboard</source>
        <translation>Буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="377"/>
        <source>Import New Key From Clipboard</source>
        <translation>Импортировать новый ключ из буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="386"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1287"/>
        <source>Key Package</source>
        <translation>Пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="387"/>
        <source>Import Key(s) From a Key Package</source>
        <translation>Импортировать ключ(и) из пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="392"/>
        <source>Export To Clipboard</source>
        <translation>Экспортировать в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="393"/>
        <source>Export Checked Key(s) To Clipboard</source>
        <translation>Экспортировать отмеченные ключи в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="398"/>
        <source>Export As Key Package</source>
        <translation>Экспортировать как пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="399"/>
        <source>Export Checked Key(s) To a Key Package</source>
        <translation>Экспортировать отмеченные ключи в пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="404"/>
        <source>Export As OpenSSH</source>
        <translation>Экспортировать в формате OpenSSH</translation>
    </message>
    <message>
        <source>Export Checked Key As OpenSSH Format to File</source>
        <translation type="vanished">Экспортировать отмеченный ключ в формате OpenSSH в файл</translation>
    </message>
    <message>
        <source>Delete Selected Key(s)</source>
        <translation type="vanished">Удалить выбранные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="414"/>
        <source>Delete the Selected keys</source>
        <translation>Удалить выбранные ключи</translation>
    </message>
    <message>
        <source>Delete Checked Key(s)</source>
        <translation type="vanished">Удалить отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="405"/>
        <source>Export a single key in OpenSSH format to a file</source>
        <translation>Экспортировать один ключ в формате OpenSSH в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="413"/>
        <source>Delete Selected Keys</source>
        <translation>Удалить выбранные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="420"/>
        <source>Refresh Key List</source>
        <translation>Обновить список ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="421"/>
        <source>Re-read the keyring from disk</source>
        <translation>Перечитать связку ключей с диска</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="429"/>
        <source>Find Key</source>
        <translation>Найти ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="429"/>
        <source>Jump to the search box</source>
        <translation>Перейти к полю поиска</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="436"/>
        <source>Delete Checked Keys</source>
        <translation>Удалить отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="437"/>
        <source>Delete the Checked keys</source>
        <translation>Удалить отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="442"/>
        <source>Show Key Details</source>
        <translation>Показать сведения о ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="443"/>
        <source>Show Details for this Key</source>
        <translation>Показать сведения об этом ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="448"/>
        <source>Set Owner Trust Level</source>
        <translation>Установить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="449"/>
        <source>Set how much you trust this key to certify others</source>
        <translation>Установить степень доверия ключу для сертификации других</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="494"/>
        <source>Copy Fingerprint</source>
        <translation>Копировать отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="495"/>
        <source>Copy this key&apos;s full fingerprint to the clipboard</source>
        <translation>Скопировать полный отпечаток этого ключа в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="499"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="502"/>
        <source>Copy Key ID</source>
        <translation>Копировать идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="503"/>
        <source>Copy this key&apos;s ID to the clipboard</source>
        <translation>Скопировать идентификатор этого ключа в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="507"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="511"/>
        <source>Copy Email</source>
        <translation>Копировать электронную почту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="512"/>
        <source>Copy this key&apos;s email address to the clipboard</source>
        <translation>Скопировать адрес электронной почты этого ключа в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="516"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="520"/>
        <source>Copy Public Key Block</source>
        <translation>Копировать блок открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="521"/>
        <source>Copy this key&apos;s armored public key block to the clipboard, ready to paste or send</source>
        <translation>Копировать ASCII-блок открытого ключа в буфер обмена для вставки или отправки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="531"/>
        <source>Certify Key...</source>
        <translation>Подписать ключ...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="532"/>
        <source>Sign this key&apos;s identity with your key</source>
        <translation>Подписать личность этого ключа вашим ключом</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="541"/>
        <source>Set Expiry...</source>
        <translation>Установить срок действия...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="541"/>
        <source>Change when this key expires</source>
        <translation>Изменить срок действия этого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="553"/>
        <source>Export Public Key...</source>
        <translation>Экспортировать открытый ключ…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="554"/>
        <source>Save this key&apos;s public half to a file you can send</source>
        <translation>Сохранить открытую часть этого ключа в файл для отправки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="563"/>
        <source>Export Private Key...</source>
        <translation>Экспортировать закрытый ключ…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="564"/>
        <source>Save this key&apos;s private half to a file — keep it to yourself</source>
        <translation>Сохранить закрытую часть этого ключа в файл — храните в тайне</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="573"/>
        <source>Generate Revocation Certificate...</source>
        <translation>Создать сертификат отзыва...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="574"/>
        <source>Create a certificate that can revoke this key, to store somewhere safe in case you ever lose control of it</source>
        <translation>Создать сертификат отзыва этого ключа для безопасного хранения на случай потери контроля над ним</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="586"/>
        <source>Search Keyserver...</source>
        <translation>Поиск на сервере ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="587"/>
        <source>Search a keyserver and import keys</source>
        <translation>Поиск на сервере ключей и импорт ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="603"/>
        <source>Publish Key to Keyserver...</source>
        <translation>Опубликовать ключ на сервере ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="610"/>
        <source>Refresh Selected from Keyserver</source>
        <translation>Обновить выбранные с сервера ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="611"/>
        <source>Fetch the latest copy of these key(s) from the key server, picking up new signatures and revocations</source>
        <translation>Загрузить последнюю версию этих ключей с сервера ключей, включая новые подписи и отзывы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="623"/>
        <source>Set Owner Trust for Checked Keys...</source>
        <translation>Установить доверие владельца для отмеченных ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="624"/>
        <source>Give every checked key the same owner trust level</source>
        <translation>Установить одинаковый уровень доверия владельца для всех отмеченных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="629"/>
        <source>Extend Expiry of Checked Keys...</source>
        <translation>Продлить срок действия отмеченных ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="630"/>
        <source>Push back the expiry date of every checked private key in one step</source>
        <translation>Продлить срок действия всех отмеченных закрытых ключей за один шаг</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="636"/>
        <source>Back Up All Private Keys...</source>
        <translation>Резервное копирование всех закрытых ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="637"/>
        <source>Export every private key in this keyring to a single key package</source>
        <translation>Экспортировать все закрытые ключи из этой связки в единый пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="652"/>
        <source>Edit</source>
        <translation>Правка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="661"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="717"/>
        <source>Key</source>
        <translation>Ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="662"/>
        <source>Generate Key</source>
        <translation>Сгенерировать ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="666"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="731"/>
        <source>Import Key</source>
        <translation>Импорт ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="671"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="737"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="738"/>
        <source>Export Key</source>
        <translation>Экспортировать ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="687"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="764"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="692"/>
        <source>Operations</source>
        <translation>Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="703"/>
        <source>Bulk</source>
        <translation>Массовые операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="732"/>
        <source>Import key</source>
        <translation>Импортировать ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="752"/>
        <source>Certify, set expiry, add a subkey, set trust, revoke</source>
        <translation>Сертифицировать, установить срок действия, добавить подключ, задать доверие, отозвать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="765"/>
        <source>Delete the selected or the checked keys</source>
        <translation>Удалить выбранные или отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="771"/>
        <source>View</source>
        <translation>Просмотр</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="886"/>
        <source>Deleting Keys</source>
        <translation>Удаление ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="887"/>
        <source>Are you sure that you want to delete the following keys?</source>
        <translation>Вы уверены, что хотите удалить следующие ключи?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="889"/>
        <source>The action can not be undone.</source>
        <translation>Это действие нельзя отменить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="941"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="953"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1012"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1240"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1251"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1264"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1296"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1304"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1320"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1328"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1356"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="967"/>
        <source>%1 is empty</source>
        <translation>%1 пуст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="971"/>
        <source>%1 copied to clipboard</source>
        <translation>%1 скопирован в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1119"/>
        <source>Nothing to Extend</source>
        <translation>Нечего продлевать</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1120"/>
        <source>None of the %n selected key(s) has a private key, so their expiry cannot be changed. Expiry is set on keys you own.</source>
        <translation>
            <numerusform>Ни один из %n выбранных ключей не имеет закрытого ключа, поэтому их срок действия не может быть изменён. Срок действия устанавливается для ключей, которыми вы владеете.</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <source>Please check or select a key before doing this operation.</source>
        <translation type="vanished">Пожалуйста, отметьте или выберите ключ перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="985"/>
        <source>Publish Key to Keyserver</source>
        <translation>Опубликовать ключ на сервере ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="987"/>
        <source>You are about to upload the following public key(s) to the default keyserver:</source>
        <translation>Вы собираетесь загрузить следующие открытые ключи на сервер ключей по умолчанию:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="990"/>
        <source>Publication is &lt;b&gt;permanent and public&lt;/b&gt;: the key(s) cannot be removed from most keyservers once uploaded. Only the public part is uploaded, never your private key.</source>
        <translation>Публикация является &lt;b&gt;постоянной и общедоступной&lt;/b&gt;: после загрузки ключи невозможно удалить с большинства серверов ключей. Загружается только открытая часть, ваш закрытый ключ никогда не передаётся.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="993"/>
        <source>Do you want to proceed?</source>
        <translation>Продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1028"/>
        <source>Upload Failed</source>
        <translation>Ошибка загрузки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1038"/>
        <source>The public key was uploaded to the key server.</source>
        <translation>Открытый ключ был загружен на сервер ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1040"/>
        <source>The public key was uploaded to %1.</source>
        <translation>Открытый ключ был загружен на %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1049"/>
        <source>Fingerprint: %1</source>
        <translation>Отпечаток: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1052"/>
        <source>Upload Complete</source>
        <translation>Загрузка завершена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1064"/>
        <source>Undefined</source>
        <translation>Не определено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1064"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1081"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1064"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1079"/>
        <source>Marginal</source>
        <translation>Предел.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1064"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1077"/>
        <source>Full</source>
        <translation>Полный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1065"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1075"/>
        <source>Ultimate</source>
        <translation>Абсолютный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1069"/>
        <source>Modify Owner Trust Level</source>
        <translation>Изменить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1070"/>
        <source>Trust level to apply to %1 checked key(s):</source>
        <translation>Уровень доверия для %1 выбранного ключа(ей):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1097"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1172"/>
        <source>Partially Failed</source>
        <translation>Частично не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1098"/>
        <source>Failed to set owner trust on %1 key(s).</source>
        <translation>Не удалось установить доверие владельца для %1 ключа(ей).</translation>
    </message>
    <message>
        <source>Please check some private keys before doing this operation.</source>
        <translation type="vanished">Пожалуйста, выберите несколько закрытых ключей перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1127"/>
        <source>Extend Expiry of Checked Keys</source>
        <translation>Продлить срок действия выбранных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1130"/>
        <source>New expiry date to apply to %1 private key(s):</source>
        <translation>Новая дата окончания срока действия для %1 закрытого ключа(ей):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1138"/>
        <source>Never expires</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1157"/>
        <source>Extending Expiry</source>
        <translation>Продление срока действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1173"/>
        <source>Failed to update expiry on %1 key(s).</source>
        <translation>Не удалось обновить срок действия для %1 ключа(ей).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1191"/>
        <source>No Private Keys</source>
        <translation>Нет закрытых ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1192"/>
        <source>There are no private keys in this keyring to back up.</source>
        <translation>В этой связке ключей нет закрытых ключей для резервного копирования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1199"/>
        <source>private key(s) backed up</source>
        <translation>Закрытые ключи сохранены в резервной копии</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1334"/>
        <source>Enter PIN</source>
        <translation>Введите PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1335"/>
        <source>Please enter PIN to decrypt the Key:</source>
        <translation>Введите PIN для расшифровки ключа:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1602"/>
        <source>Category (%1 keys)</source>
        <translation>Категория (%1 ключ(ей))</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1614"/>
        <source>Remove From This Category</source>
        <translation>Удалить из этой категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1650"/>
        <source>New Category...</source>
        <translation>Новая категория...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1654"/>
        <source>New Category</source>
        <translation>Новая категория</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1654"/>
        <source>Category name:</source>
        <translation>Название категории:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1689"/>
        <source>%1 keys · %2 private · %3 expiring soon · %4 expired or revoked</source>
        <translation>%1 ключей · %2 закрытых · %3 скоро истекающих · %4 истекших или отозванных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1696"/>
        <source>%1 key(s) expire within %2 days. See the &quot;Expiring Soon&quot; tab.</source>
        <translation>%1 ключ(ей) истекает в течение %2 дней. Смотрите вкладку «Скоро истекают».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1700"/>
        <source>No key expires within the next %1 days.</source>
        <translation>Ни один ключ не истекает в ближайшие %1 дня(ей).</translation>
    </message>
    <message>
        <source>Forbidden</source>
        <translation type="vanished">Запрещено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="604"/>
        <source>Upload the checked public key(s) to the key server configured as the default</source>
        <translation>Загрузить выбранные открытые ключи на сервер ключей, настроенный по умолчанию</translation>
    </message>
    <message>
        <source>Please check some keys before doing this operation.</source>
        <translation type="vanished">Отметьте несколько ключей перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="917"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1279"/>
        <source>key(s) exported</source>
        <translation>ключ(и) экспортированы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="931"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="998"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1230"/>
        <source>Exporting</source>
        <translation>Экспорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="942"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="954"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1013"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1241"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1252"/>
        <source>Unknown error occurred</source>
        <translation>Произошла неизвестная ошибка</translation>
    </message>
    <message>
        <source>The public key was uploaded to the key server.

Fingerprint: %1</source>
        <translation type="vanished">Открытый ключ был загружен на сервер ключей.

Отпечаток: %1</translation>
    </message>
    <message>
        <source>The public key was uploaded to %1.

Fingerprint: %2</source>
        <translation type="vanished">Открытый ключ был загружен на %1.

Отпечаток: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1211"/>
        <source>Invalid Operation</source>
        <translation>Недопустимая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1212"/>
        <source>If a key pair does not have a private key then it will not be able to generate sub-keys.</source>
        <translation>Если у пары ключей нет закрытого ключа, генерация подключей невозможна.</translation>
    </message>
    <message>
        <source>Please check a key before performing this operation.</source>
        <translation type="vanished">Отметьте ключ перед выполнением этой операции.</translation>
    </message>
    <message>
        <source>This operation accepts just a single key.</source>
        <translation type="vanished">Эта операция принимает только один ключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1265"/>
        <source>This key may not be able to export as OpenSSH format. Please check the key-size of the subkey(s) used to sign.</source>
        <translation>Этот ключ может быть невозможно экспортировать в формате OpenSSH. Проверьте размер подключей, используемых для подписи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1274"/>
        <source>Export OpenSSH Key To File</source>
        <translation>Экспортировать ключ OpenSSH в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1275"/>
        <source>OpenSSH Public Key Files</source>
        <translation>Файлы открытых ключей OpenSSH</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1287"/>
        <source>Import Key Package</source>
        <translation>Импортировать пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1297"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1321"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Невозможно открыть этот файл. Убедитесь, что это обычный файл и он доступен для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1305"/>
        <source>The target file is too large for a key package.</source>
        <translation>Целевой файл слишком большой для пакета ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1310"/>
        <source>Import Key Package Passphrase File</source>
        <translation>Импортировать файл пароля пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1311"/>
        <source>Key Package Passphrase File</source>
        <translation>Файл пароля пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1329"/>
        <source>The target file is too large for a key package passphrase.</source>
        <translation>Целевой файл слишком большой для пароля пакета ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1362"/>
        <source>key(s) imported</source>
        <translation>ключ(и) импортированы</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyNewUIDDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="45"/>
        <source>Add a new User ID</source>
        <translation>Добавить новый идентификатор пользователя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="52"/>
        <source>A User ID pairs a name with an optional email and comment. The name is required.</source>
        <translation>Идентификатор пользователя связывает имя с необязательными электронной почтой и комментарием. Имя обязательно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="58"/>
        <source>Full name</source>
        <translation>Полное имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="61"/>
        <source>name@example.com</source>
        <translation>name@example.com</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="64"/>
        <source>Optional comment</source>
        <translation>Необязательный комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="70"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="71"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="72"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="79"/>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="90"/>
        <source>The new User ID will be set as the primary User ID.</source>
        <translation>Новый идентификатор пользователя будет установлен как основной.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="139"/>
        <source>Enter a name for the User ID.</source>
        <translation>Введите имя для идентификатора пользователя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="152"/>
        <source>Please give a valid email address.</source>
        <translation>Укажите действительный адрес электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="154"/>
        <source>Ready to create the User ID.</source>
        <translation>Готов к созданию идентификатора пользователя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="196"/>
        <source>Successful Operation</source>
        <translation>Операция выполнена успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="197"/>
        <source>Successfully added a new UID.</source>
        <translation>Новый UID успешно добавлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="205"/>
        <source>Operation Failed</source>
        <translation>Операция не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="206"/>
        <source>An error occurred during the operation.</source>
        <translation>Во время операции произошла ошибка.</translation>
    </message>
    <message>
        <source>Notice: The New UID Created will be set as Primary.</source>
        <translation type="vanished">Примечание: созданный новый UID будет установлен как основной.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="116"/>
        <source>Create New UID</source>
        <translation>Создать новый UID</translation>
    </message>
    <message>
        <source>Name must contain at least five characters.</source>
        <translation type="vanished">Имя должно содержать не менее пяти символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="146"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Имя и комментарий не должны содержать символы &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; или управляющие символы.</translation>
    </message>
    <message>
        <source>Please give a email address.</source>
        <translation type="vanished">Укажите адрес электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="210"/>
        <source>Creating UID</source>
        <translation>Создание UID</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairDetailTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="63"/>
        <source>Owner</source>
        <translation>Владелец</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="64"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="65"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="66"/>
        <source>Additional UIDs</source>
        <translation>Дополнительные UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="100"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="101"/>
        <source>Email Address</source>
        <translation>Адрес электронной почты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="102"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="115"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="130"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="134"/>
        <source>Algorithm Detail</source>
        <translation>Подробности алгоритма</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="139"/>
        <source>Key Format Version</source>
        <translation>Версия формата ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="147"/>
        <source>Key Size</source>
        <translation>Размер ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="151"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="156"/>
        <source>Owner Trust Level</source>
        <translation>Уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="164"/>
        <source>Create Date (Local Time)</source>
        <translation>Дата создания (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="170"/>
        <source>Expires on (Local Time)</source>
        <translation>Истекает (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="178"/>
        <source>Last Update (Local Time)</source>
        <translation>Последнее обновление (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="183"/>
        <source>Primary Key Existence</source>
        <translation>Наличие первичного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="417"/>
        <source>Notice: The public key has been published on the key server.</source>
        <translation>Уведомление: открытый ключ был опубликован на сервере ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="419"/>
        <source>Notice: The public key has been published on %1.</source>
        <translation>Уведомление: открытый ключ был опубликован на %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="118"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="215"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="217"/>
        <source>copy fingerprint to clipboard</source>
        <translation>Копировать отпечаток в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="267"/>
        <source>Exists</source>
        <translation>Существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="267"/>
        <source>Not Exists</source>
        <translation>Не существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="290"/>
        <source>Certificate</source>
        <translation>Сертификат</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="292"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="293"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="294"/>
        <source>Auth</source>
        <translation>Аутентификация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="320"/>
        <source>Never Expire</source>
        <translation>Без срока действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="339"/>
        <source>No Data</source>
        <translation>Нет данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="353"/>
        <source>Warning: The primary key has expired.</source>
        <translation>Предупреждение: срок действия первичного ключа истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="356"/>
        <source>Warning: The primary key has been revoked.</source>
        <translation>Предупреждение: первичный ключ был отозван.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="359"/>
        <source>Warning: The primary key is not exists.</source>
        <translation>Предупреждение: первичный ключ не существует.</translation>
    </message>
    <message>
        <source>Notice: The public key has been published on keys.openpgp.org.</source>
        <translation type="vanished">Примечание: открытый ключ опубликован на keys.openpgp.org.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairOperaTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="64"/>
        <source>General Operations</source>
        <translation>Общие операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="70"/>
        <source>Export Public Key</source>
        <translation>Экспортировать открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="76"/>
        <source>Export Private Key</source>
        <translation>Экспортировать закрытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="86"/>
        <source>Modify Expiration Datetime (Primary Key)</source>
        <translation>Изменить дату истечения (первичный ключ)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="93"/>
        <source>Modify Password</source>
        <translation>Изменить пароль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="102"/>
        <source>Revoke Certificate Operation</source>
        <translation>Операция с сертификатом отзыва</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="111"/>
        <source>Set Owner Trust Level</source>
        <translation>Установить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="149"/>
        <source>Export Full Secret Key</source>
        <translation>Экспортировать полный секретный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="157"/>
        <source>Export Shortest Secret Key</source>
        <translation>Экспортировать минимальный секретный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="165"/>
        <source>Generate Revoke Certificate</source>
        <translation>Сгенерировать сертификат отзыва</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="170"/>
        <source>Import Revoke Certificate</source>
        <translation>Импортировать сертификат отзыва</translation>
    </message>
    <message>
        <source>Export Key To File</source>
        <translation type="vanished">Экспортировать ключ в файл</translation>
    </message>
    <message>
        <source>Key Files</source>
        <translation type="vanished">Файлы ключей</translation>
    </message>
    <message>
        <source>Export Error</source>
        <translation type="vanished">Ошибка экспорта</translation>
    </message>
    <message>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation type="vanished">Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <source>Export Successful</source>
        <translation type="vanished">Экспорт выполнен успешно</translation>
    </message>
    <message>
        <source>The key has been successfully exported to %1.</source>
        <translation type="vanished">Ключ успешно экспортирован в %1.</translation>
    </message>
    <message>
        <source>WARNING: You are about to export your</source>
        <translation type="vanished">ПРЕДУПРЕЖДЕНИЕ: вы собираетесь экспортировать свой</translation>
    </message>
    <message>
        <source>PRIVATE KEY</source>
        <translation type="vanished">ЗАКРЫТЫЙ КЛЮЧ</translation>
    </message>
    <message>
        <source>This is NOT your Public Key, so &lt;b&gt;DO NOT&lt;/b&gt; share it with anyone.</source>
        <translation type="vanished">Это НЕ ваш открытый ключ, поэтому &lt;b&gt;НЕ&lt;/b&gt; передавайте его никому.</translation>
    </message>
    <message>
        <source>You are exporting a &lt;b&gt;minimum size&lt;/b&gt; private key, which removes all signatures except for the latest self-signatures.</source>
        <translation type="vanished">Вы экспортируете закрытый ключ &lt;b&gt;минимального размера&lt;/b&gt;, из которого удалены все подписи, кроме последних самоподписей.</translation>
    </message>
    <message>
        <source>Do you &lt;b&gt;REALLY&lt;/b&gt; want to proceed?</source>
        <translation type="vanished">Вы &lt;b&gt;ДЕЙСТВИТЕЛЬНО&lt;/b&gt; хотите продолжить?</translation>
    </message>
    <message>
        <source>Exporting Short Private Key</source>
        <translation type="vanished">Экспорт короткого закрытого ключа</translation>
    </message>
    <message>
        <source>This operation will export your &lt;b&gt;private key&lt;/b&gt;, including both the main key and all subkeys, into an external file. This key is extremely sensitive, and anyone with access to it can impersonate you. DO NOT share this file with anyone!</source>
        <translation type="vanished">Эта операция экспортирует ваш &lt;b&gt;закрытый ключ&lt;/b&gt;, включая основной ключ и все подключи, во внешний файл. Этот ключ чрезвычайно конфиденциален, и любой, кто получит к нему доступ, сможет выдать себя за вас. НЕ передавайте этот файл никому!</translation>
    </message>
    <message>
        <source>Are you &lt;b&gt;ABSOLUTELY SURE&lt;/b&gt; you want to proceed?</source>
        <translation type="vanished">Вы &lt;b&gt;АБСОЛЮТНО УВЕРЕНЫ&lt;/b&gt;, что хотите продолжить?</translation>
    </message>
    <message>
        <source>Exporting Private Key</source>
        <translation type="vanished">Экспорт закрытого ключа</translation>
    </message>
    <message>
        <source>Unknown Error</source>
        <translation type="vanished">Неизвестная ошибка</translation>
    </message>
    <message>
        <source>Caught unknown error while exporting the key.</source>
        <translation type="vanished">Обнаружена неизвестная ошибка при экспорте ключа.</translation>
    </message>
    <message>
        <source>0 -&gt; No Reason.</source>
        <translation type="vanished">0 -&gt; Без причины.</translation>
    </message>
    <message>
        <source>1 -&gt; This key is no more safe.</source>
        <translation type="vanished">1 -&gt; Этот ключ больше не безопасен.</translation>
    </message>
    <message>
        <source>2 -&gt; Key is outdated.</source>
        <translation type="vanished">2 -&gt; Ключ устарел.</translation>
    </message>
    <message>
        <source>3 -&gt; Key is no longer used</source>
        <translation type="vanished">3 -&gt; Ключ больше не используется</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="235"/>
        <source>Revocation Certificates</source>
        <translation>Сертификаты отзыва</translation>
    </message>
    <message>
        <source>Generate revocation certificate</source>
        <translation type="vanished">Сгенерировать сертификат отзыва</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="222"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="234"/>
        <source>Import Key Revocation Certificate</source>
        <translation>Импортировать сертификат отзыва ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="223"/>
        <source>You are about to import the</source>
        <translation>Вы собираетесь импортировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="224"/>
        <source>REVOCATION CERTIFICATE</source>
        <translation>СЕРТИФИКАТ ОТЗЫВА</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="225"/>
        <source>A successful import will result in the key being irreversibly revoked.</source>
        <translation>Успешный импорт приведёт к необратимому отзыву ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="227"/>
        <source>Do you REALLY want to execute this operation?</source>
        <translation>Вы ДЕЙСТВИТЕЛЬНО хотите выполнить эту операцию?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="243"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="252"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="260"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="244"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="261"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Невозможно открыть этот файл. Убедитесь, что это обычный файл и он доступен для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="253"/>
        <source>The target file is too large for a key revocation certificate.</source>
        <translation>Целевой файл слишком большой для сертификата отзыва ключа.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairPhotosTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Flags</source>
        <translation>Флаги</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Date</source>
        <translation>Дата</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Size</source>
        <translation>Размер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="49"/>
        <source>Thumbnail</source>
        <translation>Миниатюра</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="99"/>
        <source>List of Photo IDs (%1)</source>
        <translation>Список фото-идентификаторов (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="100"/>
        <source>Photo Viewer</source>
        <translation>Просмотр фотографий</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairSubkeyTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="147"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="265"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="157"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="265"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="162"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="266"/>
        <source>Algorithm Detail</source>
        <translation>Подробности алгоритма</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="168"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="265"/>
        <source>Key Size</source>
        <translation>Размер ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="173"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="178"/>
        <source>Expires On (Local Time)</source>
        <translation>Истекает (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="185"/>
        <source>Create Date (Local Time)</source>
        <translation>Дата создания (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="189"/>
        <source>Existence</source>
        <translation>Наличие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="200"/>
        <source>Key in Smart Card</source>
        <translation>Ключ на смарт-карте</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="205"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="77"/>
        <source>List of the primary key and subkey(s)</source>
        <translation>Список первичного ключа и подключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="78"/>
        <source>Detail of Selected Primary Key/Subkey</source>
        <translation>Сведения о выбранном первичном ключе/подключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="82"/>
        <source>New Subkey</source>
        <translation>Новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="83"/>
        <source>Add ADSK(s)</source>
        <translation>Добавить ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="152"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="265"/>
        <source>Key Type</source>
        <translation>Тип ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="195"/>
        <source>Revoked</source>
        <translation>Отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="100"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="462"/>
        <source>Export Subkey</source>
        <translation>Экспортировать подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="266"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="269"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="292"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="467"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="292"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="468"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="293"/>
        <source>ADSK</source>
        <translation>ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="319"/>
        <source>Never Expire</source>
        <translation>Без срока действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="441"/>
        <source>Never Expires</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="403"/>
        <source>Exists</source>
        <translation>Существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="105"/>
        <source>Exporting subkeys is not supported by the current OpenPGP backend.</source>
        <translation>Экспорт подключей не поддерживается текущим бэкендом OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="404"/>
        <source>Not Exists</source>
        <translation>Не существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="407"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="470"/>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="407"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="470"/>
        <source>No</source>
        <translation>Нет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="461"/>
        <source>Export Primary Key</source>
        <translation>Экспортировать первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="480"/>
        <source>Edit Expire Date</source>
        <translation>Изменить дату истечения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="484"/>
        <source>Export</source>
        <translation>Экспорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="488"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="492"/>
        <source>Revoke</source>
        <translation>Отозвать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="496"/>
        <source>Change Passphrase</source>
        <translation>Изменить парольную фразу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="500"/>
        <source>Move to Card</source>
        <translation>Переместить на карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="622"/>
        <source>Exporting Subkey</source>
        <translation>Экспорт подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="623"/>
        <source>You are about to export a private subkey.</source>
        <translation>Вы собираетесь экспортировать закрытый подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="624"/>
        <source>While subkeys are less critical than the primary key, they should still be handled with care.</source>
        <translation>Хотя подключи менее критичны, чем первичный ключ, с ними всё равно следует обращаться осторожно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="627"/>
        <source>Do you want to proceed with exporting this subkey?</source>
        <translation>Хотите продолжить экспорт этого подключа?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="654"/>
        <source>Export Key To File</source>
        <translation>Экспортировать ключ в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="655"/>
        <source>Key Files</source>
        <translation>Файлы ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="660"/>
        <source>Export Error</source>
        <translation>Ошибка экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="661"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="671"/>
        <source>&lt;h3&gt;You are about to delete the subkey:&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;This action is irreversible. Please confirm.</source>
        <translation>&lt;h3&gt;Вы собираетесь удалить подключ:&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;Идентификатор:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Это действие необратимо. Пожалуйста, подтвердите.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="677"/>
        <source>Delete Subkey Confirmation</source>
        <translation>Подтверждение удаления подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="692"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="777"/>
        <source>Illegal Operation</source>
        <translation>Недопустимая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="693"/>
        <source>Cannot delete the primary key or an invalid subkey.</source>
        <translation>Невозможно удалить первичный ключ или недопустимый подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="701"/>
        <source>Operation Failed</source>
        <translation>Операция не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="702"/>
        <source>The selected subkey could not be deleted. Please check your permissions or try again.</source>
        <translation>Не удалось удалить выбранный подключ. Проверьте права доступа или попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="708"/>
        <source>Operation Successful</source>
        <translation>Операция выполнена успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="709"/>
        <source>The subkey with KeyID %1 has been successfully deleted.</source>
        <translation>Подключ с идентификатором %1 успешно удалён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="754"/>
        <source>&lt;h3&gt;Revoke Subkey Confirmation&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Revoking a subkey will make it permanently unusable. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br /&gt;Are you sure you want to revoke this subkey?</source>
        <translation>&lt;h3&gt;Подтверждение отзыва подключа&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;Идентификатор:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Отзыв подключа сделает его навсегда непригодным для использования. Это действие &lt;b&gt;необратимо&lt;/b&gt;.&lt;br /&gt;Вы уверены, что хотите отозвать этот подключ?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="761"/>
        <source>Revoke Subkey</source>
        <translation>Отозвать подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="778"/>
        <source>Cannot revoke the primary key or an invalid subkey.</source>
        <translation>Невозможно отозвать первичный ключ или недопустимый подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="783"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Без причины.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="783"/>
        <source>1 -&gt; This key is no more safe.</source>
        <translation>1 -&gt; Этот ключ больше не безопасен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="784"/>
        <source>2 -&gt; Key is outdated.</source>
        <translation>2 -&gt; Ключ устарел.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="784"/>
        <source>3 -&gt; Key is no longer used</source>
        <translation>3 -&gt; Ключ больше не используется</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="796"/>
        <source>Revocation Failed</source>
        <translation>Отзыв не удался</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="797"/>
        <source>Failed to revoke the subkey. Please try again.</source>
        <translation>Не удалось отозвать подключ. Пожалуйста, попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="800"/>
        <source>Revocation Successful</source>
        <translation>Отзыв выполнен успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="801"/>
        <source>The subkey has been successfully revoked.</source>
        <translation>Подключ успешно отозван.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairUIDTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="57"/>
        <source>New UID</source>
        <translation>Новый UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="72"/>
        <source>UIDs</source>
        <translation>Идентификаторы пользователя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="83"/>
        <source>Signature of Selected UID</source>
        <translation>Подпись выбранного UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="131"/>
        <source>Index</source>
        <translation>Индекс</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="154"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="155"/>
        <source>Expired Date</source>
        <translation>Дата истечения срока</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="261"/>
        <source>Never Expires</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="298"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="453"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="462"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="508"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="529"/>
        <source>Invalid Operation</source>
        <translation>Недопустимая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="307"/>
        <source>Are you sure that you want to delete the following UID?</source>
        <translation>Вы уверены, что хотите удалить следующий UID?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="328"/>
        <source>Successfully deleted the UID.</source>
        <translation>UID успешно удалён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="332"/>
        <source>Deleting UID</source>
        <translation>Удаление UID</translation>
    </message>
    <message>
        <source>Successfully set the Primary UID.</source>
        <translation type="vanished">Основной UID успешно установлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="372"/>
        <source>Setting Primary UID</source>
        <translation>Назначение основного UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="515"/>
        <source>&lt;h3&gt;Revoke UID Confirmation&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;UID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Revoking a UID will make it permanently unusable. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br /&gt;Are you sure you want to revoke this UID?</source>
        <translation>&lt;h3&gt;Подтверждение отзыва UID&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;UID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Отзыв UID сделает его навсегда непригодным для использования. Это действие &lt;b&gt;необратимо&lt;/b&gt;.&lt;br /&gt;Вы уверены, что хотите отозвать этот UID?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="535"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Без причины.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="535"/>
        <source>4 -&gt; User ID is no longer valid.</source>
        <translation>4 -&gt; Идентификатор пользователя больше не действителен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="550"/>
        <source>Revocation Failed</source>
        <translation>Отзыв не удался</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="551"/>
        <source>Failed to revoke the UID. Please try again.</source>
        <translation>Не удалось отозвать UID. Пожалуйста, попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="555"/>
        <source>Revocation Successful</source>
        <translation>Отзыв выполнен успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="556"/>
        <source>The UID has been successfully revoked.</source>
        <translation>UID успешно отозван.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="560"/>
        <source>Revoking UID</source>
        <translation>Отзыв UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="327"/>
        <source>Successful Operation</source>
        <translation>Успешная операция</translation>
    </message>
    <message>
        <source>Successfully added a new UID.</source>
        <translation type="vanished">Новый UID успешно добавлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="321"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="365"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="484"/>
        <source>Operation Failed</source>
        <translation>Операция не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="366"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="485"/>
        <source>An error occurred during the operation.</source>
        <translation>Произошла ошибка при выполнении операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="299"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="530"/>
        <source>Cannot delete the Primary UID.</source>
        <translation>Невозможно удалить основной UID.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="304"/>
        <source>Deleting UIDs</source>
        <translation>Удаление UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="309"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="352"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="478"/>
        <source>The action can not be undone.</source>
        <translation>Это действие нельзя отменить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="322"/>
        <source>An error occurred during the delete %1 operation.</source>
        <translation>Произошла ошибка при удалении %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="349"/>
        <source>Set Primary UID</source>
        <translation>Установить основной UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="350"/>
        <source>Are you sure that you want to set the Primary UID to?</source>
        <translation>Вы уверены, что хотите установить основной UID?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="389"/>
        <source>Set As Primary</source>
        <translation>Установить как основной</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="392"/>
        <source>Sign UID</source>
        <translation>Подписать UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="395"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="522"/>
        <source>Revoke UID</source>
        <translation>Отозвать UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="398"/>
        <source>Delete UID</source>
        <translation>Удалить UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="509"/>
        <source>Please select one UID before doing this operation.</source>
        <translation>Выберите один UID перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="442"/>
        <source>Delete(Revoke) Key Signature</source>
        <translation>Удалить (отозвать) подпись ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="454"/>
        <source>Please select one Key Signature before doing this operation.</source>
        <translation>Выберите одну подпись ключа перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="463"/>
        <source>To delete the signature, you need to have its corresponding public key in the local database.</source>
        <translation>Для удаления подписи необходимо иметь соответствующий открытый ключ в локальной базе данных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="473"/>
        <source>Deleting Key Signature</source>
        <translation>Удаление подписи ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="475"/>
        <source>Are you sure that you want to delete the following signature?</source>
        <translation>Вы уверены, что хотите удалить следующую подпись?</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeySetExpireDateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="110"/>
        <source>1 year</source>
        <translation>1 год</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="110"/>
        <source>%1 years</source>
        <translation>%1 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="113"/>
        <source>1 month</source>
        <translation>1 месяц</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="113"/>
        <source>%1 months</source>
        <translation>%1 месяцев</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="117"/>
        <source>1 day</source>
        <translation>1 день</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="117"/>
        <source>%1 days</source>
        <translation>%1 дней</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="120"/>
        <source>, </source>
        <translation>,</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="137"/>
        <source>Subkey Expiration Date</source>
        <translation>Дата истечения подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="138"/>
        <source>Key Expiration Date</source>
        <translation>Дата истечения ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="145"/>
        <source>The subkey can no longer sign or encrypt after this moment. You can extend it again later.</source>
        <translation>После этого момента подключ больше не сможет подписывать или зашифровывать. Вы сможете продлить его позже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="147"/>
        <source>The key can no longer sign or encrypt after this moment. You can extend it again later.</source>
        <translation>После этого момента ключ больше не сможет подписывать или зашифровывать. Вы сможете продлить его позже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="186"/>
        <source>Validity Period</source>
        <translation>Срок действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="187"/>
        <source>Expires On</source>
        <translation>Истекает</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="224"/>
        <source>Currently set to never expire.</source>
        <translation>Сейчас установлено без срока действия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="229"/>
        <source>Currently expired since %1.</source>
        <translation>Срок истёк с %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="231"/>
        <source>Currently expires %1 · %2 left</source>
        <translation>Истекает %1 · осталось %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="245"/>
        <source>Modify Subkey Expiration Date</source>
        <translation>Изменить дату истечения подключя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="246"/>
        <source>Modify Key Expiration Date</source>
        <translation>Изменить дату истечения ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="269"/>
        <source>Will never expire.</source>
        <translation>Никогда не истечёт.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="271"/>
        <source>The expiration date must be in the future.</source>
        <translation>Дата истечения должна быть в будущем.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="273"/>
        <source>Valid for %1 — until %2</source>
        <translation>Действителен %1 — до %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="320"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="321"/>
        <source>The expire date of the key pair has been updated.</source>
        <translation>Дата истечения пары ключей обновлена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="330"/>
        <source>Failure</source>
        <translation>Неудача</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="331"/>
        <source>Failed to update the expire date of the key pair.</source>
        <translation>Не удалось обновить дату истечения пары ключей.</translation>
    </message>
    <message>
        <source>Modified Expiration Date (Local Time)</source>
        <translation type="vanished">Изменённая дата истечения (местное время)</translation>
    </message>
    <message>
        <source>No Expiration</source>
        <translation type="vanished">Без срока действия</translation>
    </message>
    <message>
        <source>Modified Expiration Date</source>
        <translation type="vanished">Изменённая дата истечения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="120"/>
        <source>Custom Date</source>
        <translation>Пользовательская дата</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="121"/>
        <source>3 Months</source>
        <translation>3 месяца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="123"/>
        <source>6 Months</source>
        <translation>6 месяцев</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="125"/>
        <source>1 Year</source>
        <translation>1 год</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="127"/>
        <source>2 Years</source>
        <translation>2 года</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="129"/>
        <source>5 Years</source>
        <translation>5 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="131"/>
        <source>10 Years</source>
        <translation>10 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.h" line="133"/>
        <source>Never Expires</source>
        <translation>Бессрочный</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyTableEmptyState</name>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="57"/>
        <source>No keys yet.

Use Key ▸ Generate Key to make one, or Key ▸ Import Key to bring in one you already have.</source>
        <translation>Ключей пока нет.

Используйте Ключ ▸ Создать ключ, чтобы создать его, или Ключ ▸ Импортировать ключ, чтобы импортировать уже имеющийся.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="63"/>
        <source>No key matches &quot;%1&quot;.

Clear the search to see every key again.</source>
        <translation>Нет ключей, соответствующих &quot;%1&quot;.

Очистите поиск, чтобы снова увидеть все ключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="70"/>
        <source>This category has no keys yet.

Right-click a key in another tab and use Category to file it here.</source>
        <translation>В этой категории пока нет ключей.

Щёлкните правой кнопкой мыши по ключу в другой вкладке и используйте Категория, чтобы поместить его сюда.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyTableEmptyState.cpp" line="76"/>
        <source>No key in this keyring belongs in this tab.</source>
        <translation>Ни один ключ в этой связке ключей не относится к этой вкладке.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyUIDSignDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="52"/>
        <source>Signers</source>
        <translation>Подписанты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="93"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="103"/>
        <source>Sign For Key&apos;s UID(s)</source>
        <translation>Подписать UID ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="125"/>
        <source>Unsuccessful Operation</source>
        <translation>Неуспешная операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="126"/>
        <source>Signature operation failed for UID %1</source>
        <translation>Не удалось подписать UID %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="130"/>
        <source>Operation Complete</source>
        <translation>Операция завершена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="131"/>
        <source>The signature operation of the UID is complete</source>
        <translation>Операция подписи UID завершена</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::LogViewDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="90"/>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="94"/>
        <source>Application Logs</source>
        <translation>Журналы приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="100"/>
        <source>No entries</source>
        <translation>Нет записей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="110"/>
        <source>Filter logs…</source>
        <translation>Фильтр журналов…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="130"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="131"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="132"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="133"/>
        <source>Clear View</source>
        <translation>Очистить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="134"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="137"/>
        <source>Auto Refresh</source>
        <translation>Автообновление</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="249"/>
        <source>%1 entries</source>
        <translation>%1 записей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="260"/>
        <source>Save Logs</source>
        <translation>Сохранить журналы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="263"/>
        <source>Text Files (*.txt);;All Files (*)</source>
        <translation>Текстовые файлы (*.txt);;Все файлы (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="269"/>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="284"/>
        <source>Save Failed</source>
        <translation>Ошибка сохранения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="270"/>
        <source>Unable to open file for writing.</source>
        <translation>Не удалось открыть файл для записи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="285"/>
        <source>Failed to save the log file.</source>
        <translation>Не удалось сохранить файл журнала.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="289"/>
        <source>Saved</source>
        <translation>Сохранено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="290"/>
        <source>Logs have been saved successfully.</source>
        <translation>Журналы были успешно сохранены.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::MainWindow</name>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="153"/>
        <source>GUI Pinentry Not Found</source>
        <translation>Графическая программа ввода PIN не найдена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="154"/>
        <source>No suitable *graphical* Pinentry program was found on your system.

Please install a GUI-based Pinentry (e.g., &apos;pinentry-qt&apos;, &apos;pinentry-gnome3&apos;, or &apos;pinentry-mac&apos; on macOS).

Without a GUI Pinentry, GnuPG cannot prompt you for passwords or passphrases.

After installing it, please restart GpgFrontend. The configuration file will be updated automatically.</source>
        <translation>На вашей системе не найдена подходящая *графическая* программа Pinentry.

Установите графическую версию Pinentry (например, &apos;pinentry-qt&apos;, &apos;pinentry-gnome3&apos; или &apos;pinentry-mac&apos; для macOS).

Без графической программы Pinentry GnuPG не сможет запрашивать пароли и парольные фразы.

После установки перезапустите GpgFrontend. Файл конфигурации будет обновлён автоматически.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="180"/>
        <source>Add To Category</source>
        <translation>Добавить в категорию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="240"/>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="243"/>
        <source>Critical error occur while loading GpgFrontend.</source>
        <translation>Критическая ошибка при загрузке GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="242"/>
        <source>Loading Failed</source>
        <translation>Загрузка не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="374"/>
        <source>Remove From This Category</source>
        <translation>Удалить из этой категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="404"/>
        <source>New Category...</source>
        <translation>Новая категория...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="408"/>
        <source>New Category</source>
        <translation>Новая категория</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="408"/>
        <source>Category name:</source>
        <translation>Имя категории:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="440"/>
        <source>Name: %1</source>
        <translation>Имя: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="441"/>
        <source>Path: %1</source>
        <translation>Путь: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="454"/>
        <source>Invalid Key Databases</source>
        <translation>Недопустимые базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="459"/>
        <source>Detected %1 invalid key database(s).</source>
        <translation>Обнаружено %1 недопустимых баз данных ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="462"/>
        <source>The application cannot load these databases. Please review the details below and fix the issues in the GnuPG Controller.</source>
        <translation>Приложение не может загрузить эти базы данных. Просмотрите подробности ниже и устраните проблемы в контроллере GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="170"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="285"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="324"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="735"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="303"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="350"/>
        <source>Invalid KeyPair</source>
        <translation>Недопустимая пара ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="242"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="540"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="672"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1040"/>
        <source>The selected keypair cannot be used for encryption.</source>
        <translation>Выбранная пара ключей не может использоваться для шифрования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="115"/>
        <source>Instant Messaging</source>
        <translation>Мгновенные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="122"/>
        <source>Encoding</source>
        <translation>Кодировка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="124"/>
        <source>Container Format</source>
        <translation>Формат контейнера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="126"/>
        <source>Message Book</source>
        <translation>Книга сообщений</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="128"/>
        <source>Shared phrase (Argon2id)</source>
        <translation>Общая фраза (Argon2id)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="129"/>
        <source>Default — no shared phrase set</source>
        <translation>По умолчанию — общая фраза не задана</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="134"/>
        <source>Book Fingerprint</source>
        <translation>Отпечаток книги</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="139"/>
        <source>Set a Phrase</source>
        <translation>Задать фразу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="140"/>
        <source>Settings → Instant Messaging</source>
        <translation>Настройки → Мгновенные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="144"/>
        <source>OpenPGP Payload</source>
        <translation>Полезная нагрузка OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="145"/>
        <source>%1 bytes</source>
        <translation>%1 байт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="148"/>
        <source>Token Length</source>
        <translation>Длина токена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="149"/>
        <source>%1 characters</source>
        <translation>%1 символов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="157"/>
        <source>Wire Overhead</source>
        <translation>Накладные расходы канала</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="171"/>
        <source>Failed to finalize output file:

%1</source>
        <translation>Не удалось завершить создание выходного файла:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="215"/>
        <source>The operation succeeded, but GpgFrontend failed to finalize one or more output files.

Temporary output files have been cleaned up. Original files were kept unchanged.</source>
        <translation>Операция прошла успешно, но GpgFrontend не удалось завершить создание одного или нескольких выходных файлов.

Временные выходные файлы были удалены. Исходные файлы остались без изменений.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="286"/>
        <source>Cannot read from the following files:

%1</source>
        <translation>Невозможно прочитать следующие файлы:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="299"/>
        <source>The target file &quot;%1&quot; already exists.

It will only be replaced after the operation succeeds.
Do you want to continue?</source>
        <translation>Целевой файл &quot;%1&quot; уже существует.

Он будет заменен только после успешного завершения операции.
Вы хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="325"/>
        <source>Cannot write to the following files:

%1</source>
        <translation>Невозможно записать в следующие файлы:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="352"/>
        <source>For example the Following Key:</source>
        <translation>Например, следующий ключ:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="431"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="699"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="893"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="578"/>
        <source>Encrypting</source>
        <translation>Шифрование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="499"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="508"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="941"/>
        <source>Decrypting</source>
        <translation>Расшифровка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="339"/>
        <source>No Key Checked</source>
        <translation>Ключ не отмечен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="340"/>
        <source>Please check the key in the key toolbox on the right.</source>
        <translation>Отметьте ключ в панели ключей справа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="609"/>
        <source>No Message Book Phrase Set</source>
        <translation>Фраза книги сообщений не задана</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="610"/>
        <source>You have not set a Message Book phrase.</source>
        <translation>Вы не задали фразу для книги сообщений.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="611"/>
        <source>Instant messages are hidden using a shared &quot;Message Book&quot;. Without a phrase, GpgFrontend falls back to the built-in default book and that book ships in every copy of the program. It hides the format from a simple scanner, but anyone who knows GpgFrontend can still recognise your message for what it is.

Your message is OpenPGP-encrypted either way; what is at stake here is only whether it is recognisable as an encrypted message at all.

To get that, set a phrase and share it privately with the person you are writing to. You must both use exactly the same one.</source>
        <translation>Мгновенные сообщения скрываются с помощью общей «Книги сообщений». Без фразы GpgFrontend использует встроенную книгу по умолчанию, которая поставляется в каждой копии программы. Она скрывает формат от простого сканера, но любой, кто знает GpgFrontend, всё равно сможет распознать ваше сообщение.

Ваше сообщение в любом случае зашифровано OpenPGP; под вопросом только то, будет ли оно вообще распознано как зашифрованное.

Чтобы этого избежать, задайте фразу и поделитесь ею лично с получателем. Вы оба должны использовать одну и ту же фразу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="623"/>
        <source>Open Settings…</source>
        <translation>Открыть настройки…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="625"/>
        <source>Continue with Default</source>
        <translation>Продолжить с настройками по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="627"/>
        <source>Continue, Don&apos;t Ask Again</source>
        <translation>Продолжить, больше не спрашивать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="722"/>
        <source>Message Too Long</source>
        <translation>Сообщение слишком длинное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="723"/>
        <source>This message is too long to send as an instant message.

The encrypted message is %1 bytes, and the instant-messaging format carries at most %2. Shorten the text, or send it as a normal OpenPGP message instead.</source>
        <translation>Это сообщение слишком длинное для отправки как мгновенное сообщение.

Зашифрованное сообщение имеет размер %1 байт, а формат мгновенных сообщений поддерживает не более %2. Сократите текст или отправьте его как обычное сообщение OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="736"/>
        <source>Failed to prepare the instant message: the encrypted message could not be converted into a token.</source>
        <translation>Не удалось подготовить мгновенное сообщение: зашифрованное сообщение не удалось преобразовать в токен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="746"/>
        <source>Encrypt Sign</source>
        <translation>Зашифровать и подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="751"/>
        <source>Message encrypted and signed for instant messaging.</source>
        <translation>Сообщение зашифровано и подписано для мгновенной отправки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="752"/>
        <source>Message encrypted for instant messaging.</source>
        <translation>Сообщение зашифровано для мгновенной отправки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="754"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="843"/>
        <source>An Instant Messaging section followed by the OpenPGP result.</source>
        <translation>Раздел мгновенных сообщений, за которым следует результат OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="774"/>
        <source>Checking Message</source>
        <translation>Проверка сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="791"/>
        <source>Preparing Instant Message</source>
        <translation>Подготовка мгновенного сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="834"/>
        <source>Failed to decrypt instant message.</source>
        <translation>Не удалось расшифровать мгновенное сообщение.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="836"/>
        <source>Instant message decrypted (not signed).</source>
        <translation>Мгновенное сообщение расшифровано (не подписано).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="838"/>
        <source>Instant message decrypted.</source>
        <translation>Мгновенное сообщение расшифровано.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="952"/>
        <source>The selected key contains a key that does not actually have a sign usage.</source>
        <translation>Выбранный ключ содержит ключ, не имеющий возможности подписи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="481"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="980"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="650"/>
        <source>Signing</source>
        <translation>Подписание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1006"/>
        <source>File to be Verified</source>
        <translation>Файл для проверки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1007"/>
        <source>Please provide An ABSOLUTE Path 
If Data And Signature is COMBINED within a single file, KEEP THIS EMPTY: </source>
        <translation>Укажите АБСОЛЮТНЫЙ путь 
Если данные и подпись ОБЪЕДИНЕНЫ в одном файле, ОСТАВЬТЕ ПОЛЕ ПУСТЫМ: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="522"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1026"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="518"/>
        <source>Verifying</source>
        <translation>Проверка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="563"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="699"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1088"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="741"/>
        <source>Encrypting and Signing</source>
        <translation>Шифрование и подписание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="578"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="590"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1136"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="799"/>
        <source>Decrypting and Verifying</source>
        <translation>Расшифровка и проверка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="245"/>
        <source>Missing Keys</source>
        <translation>Отсутствующие ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="246"/>
        <source>Some signatures cannot be verified because the corresponding keys are missing.

The following fingerprints are missing:
%1

Would you like to fetch these keys from the key server?</source>
        <translation>Некоторые подписи невозможно проверить, так как соответствующие ключи отсутствуют.

Отсутствуют следующие отпечатки:
%1

Хотите получить эти ключи с сервера ключей?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="267"/>
        <source>Verification Incomplete</source>
        <translation>Проверка не завершена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="268"/>
        <source>Verification was incomplete due to missing keys. You can manually import the keys later.</source>
        <translation>Проверка не завершена из-за отсутствия ключей. Вы можете импортировать ключи позже вручную.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="85"/>
        <source>Localize</source>
        <translation>Локализация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="88"/>
        <source>UTC</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="99"/>
        <source>Local Time</source>
        <translation>Местное время</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="313"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="290"/>
        <source>No operation result available.</source>
        <translation>Результат операции недоступен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="309"/>
        <source>FAILED</source>
        <translation>ОШИБКА</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="316"/>
        <source>WARNING</source>
        <translation>ПРЕДУПРЕЖДЕНИЕ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="322"/>
        <source>Object: %1</source>
        <translation>Объект: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="338"/>
        <source>Summary</source>
        <translation>Сводка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="340"/>
        <source>Total operations: %1</source>
        <translation>Всего операций: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="346"/>
        <source>Failed objects: %1</source>
        <translation>Неудачных объектов: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="350"/>
        <source>Warning objects: %1</source>
        <translation>Объектов с предупреждениями: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="355"/>
        <source>Details</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="418"/>
        <source>Email Operation Failed</source>
        <translation>Ошибка операции электронной почты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="424"/>
        <source>The operation could not be completed. See the reason and suggested solutions below.</source>
        <translation>Не удалось выполнить операцию. См. причину и предлагаемые решения ниже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="426"/>
        <source>Report</source>
        <translation>Отчёт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="459"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="504"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="552"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="617"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="689"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="782"/>
        <source>Unsupported Operation</source>
        <translation>Неподдерживаемая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="460"/>
        <source>The decrypt operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция расшифровки для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="505"/>
        <source>The verify operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция проверки для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="553"/>
        <source>The encryption operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция шифрования для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="560"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="627"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="699"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="718"/>
        <source>No Key Selected</source>
        <translation>Ключ не выбран</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="561"/>
        <source>Please select a key for encryption.</source>
        <translation>Выберите ключ для шифрования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="618"/>
        <source>The sign operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция подписи для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="690"/>
        <source>The encrypt and sign operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция шифрования и подписи для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="700"/>
        <source>Please select a key for encrypt the EML.</source>
        <translation>Выберите ключ для шифрования EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="783"/>
        <source>The decrypt and verify operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция расшифровки и проверки для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="628"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="719"/>
        <source>Please select a key for signing the EML.</source>
        <translation>Выберите ключ для подписи EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="341"/>
        <source>Successful: %1</source>
        <translation>Успешно: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="342"/>
        <source>Warnings: %1</source>
        <translation>Предупреждений: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="343"/>
        <source>Failures: %1</source>
        <translation>Неудач: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="633"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="724"/>
        <source>Multiple Keys Selected</source>
        <translation>Выбрано несколько ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="634"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="725"/>
        <source>Please select only one key to sign the EML.</source>
        <translation>Выберите только один ключ для подписи EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="77"/>
        <source>Save File</source>
        <translation>Сохранить файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="78"/>
        <source>Save the current File</source>
        <translation>Сохранить текущий файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="82"/>
        <source>Save As</source>
        <translation>Сохранить как</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="83"/>
        <source>Save the current File as...</source>
        <translation>Сохранить текущий файл как...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="86"/>
        <source>Print</source>
        <translation>Печать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="87"/>
        <source>Print Document</source>
        <translation>Печать документа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="142"/>
        <source>Quit</source>
        <translation>Выход</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="143"/>
        <source>Quit Program</source>
        <translation>Выход из программы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="147"/>
        <source>Undo</source>
        <translation>Отменить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="148"/>
        <source>Undo Last Edit Action</source>
        <translation>Отменить последнее действие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="151"/>
        <source>Redo</source>
        <translation>Повторить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="152"/>
        <source>Redo Last Edit Action</source>
        <translation>Повторить последнее действие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="155"/>
        <source>Zoom In</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="156"/>
        <source>Zoom in</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="160"/>
        <source>Zoom Out</source>
        <translation>Уменьшить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="161"/>
        <source>Zoom out</source>
        <translation>Уменьшить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="165"/>
        <source>Paste</source>
        <translation>Вставить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="166"/>
        <source>Paste Text From Clipboard</source>
        <translation>Вставить текст из буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="170"/>
        <source>Cut</source>
        <translation>Вырезать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="171"/>
        <source>Cut the current selection&apos;s contents to the clipboard</source>
        <translation>Вырезать содержимое выделенного в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="176"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="177"/>
        <source>Copy the current selection&apos;s contents to the clipboard</source>
        <translation>Копировать содержимое выделенного в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="181"/>
        <source>Quote</source>
        <translation>Цитировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="182"/>
        <source>Quote whole text</source>
        <translation>Цитировать весь текст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="186"/>
        <source>Select All</source>
        <translation>Выделить всё</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="187"/>
        <source>Select the whole text</source>
        <translation>Выделить весь текст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="191"/>
        <source>Find</source>
        <translation>Найти</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="192"/>
        <source>Find a word</source>
        <translation>Найти слово</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="196"/>
        <source>Remove spacing</source>
        <translation>Удалить пробелы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="198"/>
        <source>Remove double linebreaks, e.g. in pasted text from Web Mailer</source>
        <translation>Удалить двойные переносы строк, например, в тексте из веб-почты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="203"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="204"/>
        <source>Open settings dialog</source>
        <translation>Открыть диалог настроек</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="746"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="212"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="213"/>
        <source>Encrypt Message</source>
        <translation>Зашифровать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="220"/>
        <source>Encrypt and Sign Message</source>
        <translation>Зашифровать и подписать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="825"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="225"/>
        <source>Decrypt</source>
        <translation>Расшифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="226"/>
        <source>Decrypt Message</source>
        <translation>Расшифровать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="233"/>
        <source>Decrypt and Verify Message</source>
        <translation>Расшифровать и проверить сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="238"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="239"/>
        <source>Sign Message</source>
        <translation>Подписать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="243"/>
        <source>Verify</source>
        <translation>Проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="244"/>
        <source>Verify Message</source>
        <translation>Проверить сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="250"/>
        <source>Sym. Encrypt</source>
        <translation>Симм. шифрование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="251"/>
        <source>Encrypt Message (Symmetric)</source>
        <translation>Зашифровать сообщение (симметрично)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="261"/>
        <source>New Keypair</source>
        <translation>Новая пара ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="262"/>
        <source>Generate KeyPair</source>
        <translation>Сгенерировать пару ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="269"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="423"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="565"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="271"/>
        <source>Import New Key From File</source>
        <translation>Импортировать новый ключ из файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="278"/>
        <source>Clipboard</source>
        <translation>Буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="280"/>
        <source>Import New Key From Clipboard</source>
        <translation>Импортировать новый ключ из буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="287"/>
        <source>Editor</source>
        <translation>Редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="288"/>
        <source>Import New Key From Editor</source>
        <translation>Импортировать новый ключ из редактора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="293"/>
        <source>Manage Keys</source>
        <translation>Управление ключами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="294"/>
        <source>Open Key Management</source>
        <translation>Открыть управление ключами</translation>
    </message>
    <message>
        <source>Clear Password Cache</source>
        <translation type="vanished">Очистить кэш паролей</translation>
    </message>
    <message>
        <source>Clear Password Cache of GnuPG</source>
        <translation type="vanished">Очистить кэш паролей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="305"/>
        <source>Open Smart Card Controller</source>
        <translation>Открыть управление смарт-картами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="306"/>
        <source>Open Smart Card Controller Dialog</source>
        <translation>Открыть диалог управления смарт-картами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="404"/>
        <source>IM Encrypt</source>
        <translation>IM-шифрование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="405"/>
        <source>Encrypt the current text into a compact, single-line format that is safe to paste into instant messengers. Recipients decrypt it with the normal Decrypt action.</source>
        <translation>Зашифровать текущий текст в компактный однострочный формат, безопасный для вставки в мессенджеры. Получатели расшифровывают его обычным действием «Расшифровать».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="413"/>
        <source>IM Encrypt &amp;&amp; Sign</source>
        <translation>IM-шифрование &amp;&amp; подписание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="414"/>
        <source>Encrypt and sign the current text into a compact, single-line format that is safe to paste into instant messengers. Recipients decrypt and verify it with the normal Decrypt &amp; Verify action.</source>
        <translation>Зашифровать и подписать текущий текст в компактный однострочный формат, безопасный для вставки в мессенджеры. Получатели расшифровывают и проверяют его обычным действием «Расшифровать и проверить».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="425"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="571"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="430"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="577"/>
        <source>Workspace</source>
        <translation>Рабочая область</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="502"/>
        <source>Profiles</source>
        <translation>Профили</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="507"/>
        <source>Open Recent</source>
        <translation>Открыть недавние</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="523"/>
        <source>Advanced</source>
        <translation>Расширенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="571"/>
        <source>Open a file or directory</source>
        <translation>Открыть файл или каталог</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="578"/>
        <source>Open a text editor or file panel</source>
        <translation>Открыть текстовый редактор или панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="598"/>
        <source>Import</source>
        <translation>Импорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="599"/>
        <source>Import a key from file, editor, or clipboard</source>
        <translation>Импортировать ключ из файла, редактора или буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="617"/>
        <source>Text Tools</source>
        <translation>Текстовые инструменты</translation>
    </message>
    <message>
        <source>Profile: %1  (temporary)</source>
        <translation type="vanished">Профиль: %1 (временный)</translation>
    </message>
    <message>
        <source>Profile: %1</source>
        <translation type="vanished">Профиль: %1</translation>
    </message>
    <message>
        <source>Opened from a file, and not kept on this computer. Closing asks whether to save the changes back into it.</source>
        <translation type="vanished">Открыт из файла и не хранится на этом компьютере. При закрытии будет предложено сохранить изменения обратно в него.</translation>
    </message>
    <message>
        <source>This window&apos;s profile — its own settings, keys and saved state</source>
        <translation type="vanished">Профиль этого окна — его собственные настройки, ключи и сохранённое состояние</translation>
    </message>
    <message>
        <source>Current OpenPGP backend and version</source>
        <translation type="vanished">Текущий бэкенд OpenPGP и его версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="744"/>
        <source>Status Panel</source>
        <translation>Панель состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="145"/>
        <source>Open Profile File</source>
        <translation>Открыть файл профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="175"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="209"/>
        <source>Cannot Open Profile</source>
        <translation>Не удаётся открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="187"/>
        <source>Nothing opened yet</source>
        <translation>Ещё ничего не открыто</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="200"/>
        <source>%1  (open in another window)</source>
        <translation>%1 (открыт в другом окне)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="271"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="290"/>
        <source>Cannot Export Profile</source>
        <translation>Не удаётся экспортировать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="272"/>
        <source>The application key is not available, so the profile could not be packed.</source>
        <translation>Ключ приложения недоступен, поэтому профиль не может быть упакован.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="280"/>
        <source>Exporting Profile</source>
        <translation>Экспорт профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="296"/>
        <source>Profile Exported</source>
        <translation>Профиль экспортирован</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="297"/>
        <source>&quot;%1&quot; was written to:</source>
        <translation>&quot;%1&quot; записан в:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="302"/>
        <source>It can only be opened with the passphrase you chose. There is no way to recover it.</source>
        <translation>Его можно открыть только с выбранной вами парольной фразой. Восстановить её невозможно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="304"/>
        <source>It is not protected: anyone who gets this file can read the keys inside it.</source>
        <translation>Он не защищён: любой, кто получит этот файл, сможет прочитать ключи внутри него.</translation>
    </message>
    <message>
        <source>Successful Operation</source>
        <translation type="vanished">Успешная операция</translation>
    </message>
    <message>
        <source>Clear password cache successfully</source>
        <translation type="vanished">Кэш паролей успешно очищен</translation>
    </message>
    <message>
        <source>Failed Operation</source>
        <translation type="vanished">Неудавшаяся операция</translation>
    </message>
    <message>
        <source>Failed to clear password cache of GnuPG</source>
        <translation type="vanished">Не удалось очистить кэш паролей GnuPG</translation>
    </message>
    <message>
        <source>Engine: %1</source>
        <translation type="vanished">Движок: %1</translation>
    </message>
    <message>
        <source>Engine: %1 %2</source>
        <translation type="vanished">Движок: %1 %2</translation>
    </message>
    <message>
        <source>Reload All Components</source>
        <translation type="vanished">Перезагрузить все компоненты</translation>
    </message>
    <message>
        <source>Reload All GnuPG&apos;s Components</source>
        <translation type="vanished">Перезагрузить все компоненты GnuPG</translation>
    </message>
    <message>
        <source>Reload all the GnuPG&apos;s components successfully</source>
        <translation type="vanished">Все компоненты GnuPG успешно перезагружены</translation>
    </message>
    <message>
        <source>Failed to reload all or one of the GnuPG&apos;s component(s)</source>
        <translation type="vanished">Не удалось перезагрузить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <source>Restart All Components</source>
        <translation type="vanished">Перезапустить все компоненты</translation>
    </message>
    <message>
        <source>Restart All GnuPG&apos;s Components</source>
        <translation type="vanished">Перезапустить все компоненты GnuPG</translation>
    </message>
    <message>
        <source>Restart all the GnuPG&apos;s components successfully</source>
        <translation type="vanished">Все компоненты GnuPG успешно перезапущены</translation>
    </message>
    <message>
        <source>Failed to restart all or one of the GnuPG&apos;s component(s)</source>
        <translation type="vanished">Не удалось перезапустить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="54"/>
        <source>New Text Editor</source>
        <translation>Новый текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="60"/>
        <source>New File Panel</source>
        <translation>Новая панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="66"/>
        <source>File...</source>
        <translation>Файл...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="67"/>
        <source>Open a file in the file panel</source>
        <translation>Открыть файл на панели файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="72"/>
        <source>Directory...</source>
        <translation>Директория...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="73"/>
        <source>Open a directory in the file panel</source>
        <translation>Открыть директорию на панели файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="91"/>
        <source>Close Tab</source>
        <translation>Закрыть вкладку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="92"/>
        <source>Close the current tab</source>
        <translation>Закрыть текущую вкладку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="97"/>
        <source>Manage Profiles...</source>
        <translation>Управление профилями…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="98"/>
        <source>See every profile on this computer, and open, rename or remove one</source>
        <translation>Просмотреть все профили на этом компьютере, а также открыть, переименовать или удалить один из них.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="105"/>
        <source>New Profile...</source>
        <translation>Новый профиль…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="106"/>
        <source>Start an empty profile on this computer, with its own settings and keys</source>
        <translation>Создать пустой профиль на этом компьютере с собственными настройками и ключами.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="117"/>
        <source>Open Profile File...</source>
        <translation>Открыть файл профиля…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="118"/>
        <source>Work inside a profile file, leaving it a file. Nothing is added to this computer, and your changes go back into the same file.</source>
        <translation>Работать внутри файла профиля, оставляя его файлом. Ничего не добавляется на этот компьютер, а ваши изменения сохраняются обратно в тот же файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="129"/>
        <source>Import Profile File...</source>
        <translation>Импортировать файл профиля…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="130"/>
        <source>Copy a profile file into a new profile kept on this computer. The file is not used again afterwards.</source>
        <translation>Скопировать файл профиля в новый профиль на этом компьютере. После этого файл больше не используется.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="136"/>
        <source>Export This Profile...</source>
        <translation>Экспортировать этот профиль...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="137"/>
        <source>Write the profile this window is using out to a new profile file, to carry elsewhere or keep as a backup</source>
        <translation>Записать профиль, который использует это окно, в новый файл профиля — чтобы перенести его в другое место или сохранить как резервную копию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="219"/>
        <source>Encrypt &amp;&amp; Sign</source>
        <translation>Зашифровать &amp;&amp; Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="232"/>
        <source>Decrypt &amp;&amp; Verify</source>
        <translation>Расшифровать &amp;&amp; Проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="299"/>
        <source>Open Module Controller</source>
        <translation>Открыть контроллер модулей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="300"/>
        <source>Open Module Controller Dialog</source>
        <translation>Открыть диалог контроллера модулей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="313"/>
        <source>About</source>
        <translation>О программе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="314"/>
        <source>Show the application&apos;s About box</source>
        <translation>Показать окно «О программе»</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="324"/>
        <source>Open Wizard</source>
        <translation>Открыть мастер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="325"/>
        <source>Open the wizard</source>
        <translation>Открыть мастер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="330"/>
        <source>Show Application Log</source>
        <translation>Показать журнал приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="331"/>
        <source>Show the application log view</source>
        <translation>Показать окно журнала приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="336"/>
        <source>Append Public Key to Editor</source>
        <translation>Добавить открытый ключ в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="337"/>
        <source>Append selected Keypair&apos;s Public Key to Editor</source>
        <translation>Добавить открытый ключ выбранной пары ключей в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="342"/>
        <source>Append Create DateTime to Editor</source>
        <translation>Добавить дату создания в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="343"/>
        <source>Append selected Key&apos;s creation date and time to Editor</source>
        <translation>Добавить дату и время создания выбранного ключа в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="348"/>
        <source>Append Expire DateTime to Editor</source>
        <translation>Добавить дату истечения в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="349"/>
        <source>Append selected Key&apos;s expiration date and time to Editor</source>
        <translation>Добавить дату и время истечения срока выбранного ключа в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="354"/>
        <source>Append Fingerprint to Editor</source>
        <translation>Добавить отпечаток в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="355"/>
        <source>Append selected Key&apos;s Fingerprint to Editor</source>
        <translation>Добавить отпечаток выбранного ключа в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="360"/>
        <source>Copy Email</source>
        <translation>Копировать email</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="361"/>
        <source>Copy selected Keypair&apos;s to clipboard</source>
        <translation>Копировать выбранную пару ключей в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="366"/>
        <source>Copy Default UID</source>
        <translation>Копировать UID по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="367"/>
        <source>Copy selected Keypair&apos;s default UID to clipboard</source>
        <translation>Копировать UID по умолчанию выбранной пары ключей в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="372"/>
        <source>Copy Key ID</source>
        <translation>Копировать идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="373"/>
        <source>Copy selected Keypair&apos;s ID to clipboard</source>
        <translation>Копировать идентификатор выбранной пары ключей в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="378"/>
        <source>Show Key Details</source>
        <translation>Показать сведения о ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="379"/>
        <source>Show Details for this Key</source>
        <translation>Показать сведения об этом ключе</translation>
    </message>
    <message>
        <source>Add To Favourite</source>
        <translation type="vanished">Добавить в избранное</translation>
    </message>
    <message>
        <source>Add this key to Favourite Table</source>
        <translation type="vanished">Добавить этот ключ в таблицу избранного</translation>
    </message>
    <message>
        <source>Remove From Favourite</source>
        <translation type="vanished">Удалить из избранного</translation>
    </message>
    <message>
        <source>Remove this key from Favourite Table</source>
        <translation type="vanished">Удалить этот ключ из таблицы избранного</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="384"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="385"/>
        <source>Set Owner Trust Level</source>
        <translation>Установить уровень доверия владельца</translation>
    </message>
    <message>
        <source>Remove PGP Header</source>
        <translation type="vanished">Удалить заголовок PGP</translation>
    </message>
    <message>
        <source>Add PGP Header</source>
        <translation type="vanished">Добавить заголовок PGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="445"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="608"/>
        <source>Edit</source>
        <translation>Редактировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="476"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="592"/>
        <source>Keys</source>
        <translation>Ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="478"/>
        <source>Import Key</source>
        <translation>Импорт ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="538"/>
        <source>View</source>
        <translation>Вид</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="540"/>
        <source>Help</source>
        <translation>Помощь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="463"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="587"/>
        <source>Operations</source>
        <translation>Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="55"/>
        <source>Open a new text editor</source>
        <translation>Открыть новый текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="61"/>
        <source>Open a new file panel</source>
        <translation>Открыть новую панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="667"/>
        <source>Ready</source>
        <translation>Готово</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="671"/>
        <source>Key ToolBox</source>
        <translation>Панель ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="696"/>
        <source>Default</source>
        <translation>По умолчанию</translation>
    </message>
    <message>
        <source>Favourite</source>
        <translation type="vanished">Избранное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="704"/>
        <source>Key Group</source>
        <translation>Группа ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="711"/>
        <source>Only Public Key</source>
        <translation>Только открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="719"/>
        <source>Has Private Key</source>
        <translation>Имеет закрытый ключ</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleControllerDialog</name>
    <message>
        <source>Module Information</source>
        <translation type="vanished">Информация о модуле</translation>
    </message>
    <message>
        <source>Actions</source>
        <translation type="vanished">Действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="118"/>
        <source>Show Mods Directory</source>
        <translation>Показать каталог модулей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="87"/>
        <source>Registered Modules</source>
        <translation>Зарегистрированные модули</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="88"/>
        <source>Global Register Table</source>
        <translation>Глобальная таблица регистрации</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="89"/>
        <source>Debugger</source>
        <translation>Отладчик</translation>
    </message>
    <message>
        <source>Tips: Module name front with &quot;*&quot; stands for integrated module.</source>
        <translation type="vanished">Подсказка: имя модуля с «*» означает встроенный модуль.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="85"/>
        <source>Module Controller</source>
        <translation>Контроллер модулей</translation>
    </message>
    <message>
        <source>Enable Auto Activate</source>
        <translation type="vanished">Включить автоактивацию</translation>
    </message>
    <message>
        <source>BASIC INFO</source>
        <translation type="vanished">ОСНОВНАЯ ИНФОРМАЦИЯ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="106"/>
        <source>ID</source>
        <translation>Идентификатор</translation>
    </message>
    <message>
        <source>Version</source>
        <translation type="vanished">Версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="91"/>
        <source>Search modules...</source>
        <translation>Поиск модулей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="92"/>
        <source>All</source>
        <translation>Все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="96"/>
        <source>Inactive</source>
        <translation>Неактивные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="98"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="294"/>
        <source>Integrated</source>
        <translation>Встроенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="100"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="294"/>
        <source>External</source>
        <translation>Внешние</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="104"/>
        <source>Select a module to see its details.</source>
        <translation>Выберите модуль для просмотра его сведений.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="107"/>
        <source>SDK Version</source>
        <translation>Версия SDK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="108"/>
        <source>Qt ENV Version</source>
        <translation>Версия Qt ENV</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="109"/>
        <source>Hash</source>
        <translation>Хеш</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="110"/>
        <source>Path</source>
        <translation>Путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="112"/>
        <source>Listening Events</source>
        <translation>Прослушиваемые события</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="114"/>
        <source>Activate on Start</source>
        <translation>Активировать при запуске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="116"/>
        <source>Activate this module automatically when GpgFrontend starts.</source>
        <translation>Автоматически активировать этот модуль при запуске GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="117"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="123"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="120"/>
        <source>Search keys and values...</source>
        <translation>Поиск ключей и значений...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="121"/>
        <source>Expand All</source>
        <translation>Развернуть всё</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="122"/>
        <source>Collapse All</source>
        <translation>Свернуть все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="125"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="202"/>
        <source>Trigger Event</source>
        <translation>Активировать событие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="126"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="211"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="215"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="219"/>
        <source>Upsert GRT Value</source>
        <translation>Обновить значение GRT</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="136"/>
        <source>%1 modules · %2 active</source>
        <translation>Модули: %1 · Активно: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="203"/>
        <source>Please provide an Event ID</source>
        <translation>Укажите идентификатор события</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="211"/>
        <source>Namespace</source>
        <translation>Пространство имён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="215"/>
        <source>Key</source>
        <translation>Ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="220"/>
        <source>Value</source>
        <translation>Значение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="236"/>
        <source>Module loading is disabled. Enable it in Settings &gt; General to use modules.</source>
        <translation>Загрузка модулей отключена. Включите её в настройках &gt; Общие, чтобы использовать модули.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="240"/>
        <source>Only integrated modules are loaded. To load external modules from the mods directory, change the module loading policy in Settings &gt; General.</source>
        <translation>Загружены только встроенные модули. Чтобы загрузить внешние модули из каталога mods, измените политику загрузки модулей в настройках &gt; Общие.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="292"/>
        <source>● Active</source>
        <translation>● Активный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="292"/>
        <source>○ Inactive</source>
        <translation>○ Неактивный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="298"/>
        <source>Auto Start</source>
        <translation>Автозапуск</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="303"/>
        <source>by %1</source>
        <translation>от %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="316"/>
        <source>N/A</source>
        <translation>Н/Д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="327"/>
        <source>N/A (integrated)</source>
        <translation>Н/Д (встроенный)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="331"/>
        <source>Click to open the containing folder</source>
        <translation>Нажмите, чтобы открыть папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="341"/>
        <source>Listening Events (%1)</source>
        <translation>Прослушиваемые события (%1)</translation>
    </message>
    <message>
        <source>Auto Activate</source>
        <translation type="vanished">Автоактивация</translation>
    </message>
    <message>
        <source>True</source>
        <translation type="vanished">Да</translation>
    </message>
    <message>
        <source>False</source>
        <translation type="vanished">Нет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="94"/>
        <source>Active</source>
        <translation>Активен</translation>
    </message>
    <message>
        <source>METADATA</source>
        <translation type="vanished">МЕТАДАННЫЕ</translation>
    </message>
    <message>
        <source>Listening Event</source>
        <translation type="vanished">Прослушиваемое событие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="343"/>
        <source>Deactivate</source>
        <translation>Деактивировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="344"/>
        <source>Activate</source>
        <translation>Активировать</translation>
    </message>
    <message>
        <source>Disable Auto Activate</source>
        <translation type="vanished">Отключить автоактивацию</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleItemDelegate</name>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="196"/>
        <source>Integrated</source>
        <translation>Встроенный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="196"/>
        <source>External</source>
        <translation>Внешний</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="202"/>
        <source>Auto</source>
        <translation>Авто</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::MoveKeyToCardPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="120"/>
        <source>Select Key to Move to Card</source>
        <translation>Выберите ключ для перемещения на смарт-карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="125"/>
        <source>Choose a Key or Subkey to Move</source>
        <translation>Выберите ключ или подключ для перемещения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="131"/>
        <source>Select the single private key or subkey to move onto the smart card. Expand a key to choose a particular subkey. Only parts that can be stored on a card are selectable.</source>
        <translation>Выберите один закрытый ключ или подключ для перемещения на смарт-карту. Разверните ключ, чтобы выбрать конкретный подключ. Выбирать можно только те части, которые можно сохранить на смарт-карте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="145"/>
        <source>Database</source>
        <translation>База данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="175"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="178"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="183"/>
        <source>Select One Key</source>
        <translation>Выберите один ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="184"/>
        <source>Please select exactly one key or subkey to move to the card.</source>
        <translation>Пожалуйста, выберите ровно один ключ или подключ для перемещения на смарт-карту.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::NetworkTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="65"/>
        <source>Proxy</source>
        <translation>Прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="66"/>
        <source>Network Ability</source>
        <translation>Сетевые возможности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="67"/>
        <source>Operations</source>
        <translation>Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="69"/>
        <source>Enable Proxy</source>
        <translation>Включить прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="70"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="73"/>
        <source>Port</source>
        <translation>Порт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="72"/>
        <source>Host Address</source>
        <translation>Адрес хоста</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="74"/>
        <source>Proxy Type</source>
        <translation>Тип прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="75"/>
        <source>Username</source>
        <translation>Имя пользователя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="76"/>
        <source>Password</source>
        <translation>Пароль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="79"/>
        <source>Apply Proxy Settings and Check Proxy Connection</source>
        <translation>Применить настройки прокси и проверить подключение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="81"/>
        <source>Automatically fetch key publish status from key server.</source>
        <translation>Автоматически получать статус публикации ключа с сервера ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="170"/>
        <source>Test Server Url Accessibility</source>
        <translation>Проверить доступность URL сервера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="171"/>
        <source>Server Url</source>
        <translation>URL сервера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="181"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="182"/>
        <source>Successfully connect to the target server through the proxy server.</source>
        <translation>Успешное подключение к целевому серверу через прокси-сервер.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="186"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="187"/>
        <source>Unable to connect to the target server through the proxy server. Proxy settings may be invalid.</source>
        <translation>Не удалось подключиться к целевому серверу через прокси-сервер. Настройки прокси могут быть недопустимы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="197"/>
        <source>Test Proxy Server Connection...</source>
        <translation>Тест подключения к прокси-серверу...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="198"/>
        <source>Is using your proxy settings to access the url. Note that this test operation will apply your proxy settings to the entire software.</source>
        <translation>Используются ваши настройки прокси для доступа к URL. Обратите внимание, что эта тестовая операция применит настройки прокси ко всему приложению.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::PassphraseDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="39"/>
        <source>Passphrase Required</source>
        <translation>Требуется парольная фраза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="48"/>
        <source>Enter Passphrase</source>
        <translation>Введите парольную фразу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="55"/>
        <source>Please enter the passphrase required for the current operation.</source>
        <translation>Пожалуйста, введите парольную фразу, необходимую для текущей операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="64"/>
        <source>Incorrect Passphrase</source>
        <translation>Неверная парольная фраза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="67"/>
        <source>The passphrase you entered was incorrect. Please try again.</source>
        <translation>Введенная вами парольная фраза неверна. Пожалуйста, попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="90"/>
        <source>Passphrase info: %1</source>
        <translation>Информация о парольной фразе: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="94"/>
        <source>This passphrase will be used to set a new password.</source>
        <translation>Эта парольная фраза будет использована для установки нового пароля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="99"/>
        <source>Key ID: %1</source>
        <translation>Идентификатор ключа: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="100"/>
        <source>Key UID: %1</source>
        <translation>UID ключа: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="117"/>
        <source>Enter your passphrase here</source>
        <translation>Введите вашу парольную фразу здесь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="124"/>
        <source>Enter your passphrase again</source>
        <translation>Введите вашу парольную фразу еще раз</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="134"/>
        <source>Show</source>
        <translation>Показать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="154"/>
        <source>Passphrase:</source>
        <translation>Парольная фраза:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="174"/>
        <source>Strength:</source>
        <translation>Надёжность:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="182"/>
        <source>Confirm:</source>
        <translation>Подтверждение:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="194"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="195"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="277"/>
        <source>Empty Passphrase</source>
        <translation>Пустая парольная фраза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="278"/>
        <source>Passphrase cannot be empty. Please enter a valid passphrase.</source>
        <translation>Парольная фраза не может быть пустой. Пожалуйста, введите корректную парольную фразу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="287"/>
        <source>Passphrase Mismatch</source>
        <translation>Несовпадение парольных фраз</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="288"/>
        <source>The two passphrases do not match. Please enter them again.</source>
        <translation>Введенные парольные фразы не совпадают. Пожалуйста, введите их еще раз.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseDialog.cpp" line="317"/>
        <source>Closing in %1:%2</source>
        <translation>Закрытие через %1:%2</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::PlainTextEditorPage</name>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="76"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="118"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="184"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="328"/>
        <source>Loading...</source>
        <translation>Загрузка...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="120"/>
        <source>Ln 1, Col 1 · 0 chars</source>
        <translation>Стр. 1, Стлб. 1 · 0 симв.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="121"/>
        <source>Number of characters in the editor.</source>
        <translation>Количество символов в редакторе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="123"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="208"/>
        <source>LF</source>
        <translation>LF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="124"/>
        <source>Line ending style.</source>
        <translation>Стиль окончания строк.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="128"/>
        <source>Text encoding.</source>
        <translation>Кодировка текста.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="138"/>
        <source>SHA-256 checksum of editor content.</source>
        <translation>Контрольная сумма SHA-256 содержимого редактора.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="202"/>
        <source>Ln %1, Col %2 · %3 chars%4</source>
        <translation>Стр. %1, Стлб. %2 · %3 симв.%4</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="208"/>
        <source>CRLF</source>
        <translation>CRLF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="214"/>
        <source>The document has unsaved changes.</source>
        <translation>В документе есть несохраненные изменения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="215"/>
        <source>The document is unchanged.</source>
        <translation>Документ не изменен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="397"/>
        <source>Loading... %1 KB</source>
        <translation>Загрузка... %1 КБ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="127"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="209"/>
        <source>UTF-8</source>
        <translation>UTF-8</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileCreateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="65"/>
        <source>New Profile</source>
        <translation>Новый профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="87"/>
        <source>for example: Work</source>
        <translation>например: Работа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="88"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="91"/>
        <source>Keys</source>
        <translation>Ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="94"/>
        <source>Start with an empty keyring</source>
        <translation>Начать с пустой связки ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="97"/>
        <source>Keys live inside this profile. It stays separate from everything else and can be exported as a single file. It opens with no keys, so you will import or create them here.</source>
        <translation>Ключи хранятся внутри этого профиля. Он отделен от всего остального и может быть экспортирован в виде одного файла. Он открывается без ключей, поэтому вы будете импортировать или создавать их здесь.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="103"/>
        <source>Use the system GnuPG keyring</source>
        <translation>Использовать системную связку ключей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="105"/>
        <source>The same keys you already have. They are shared with the rest of the system, so they are not really separate and cannot be carried inside a profile file.</source>
        <translation>Те же ключи, которые у вас уже есть. Они являются общими для всей системы, поэтому не являются по-настоящему отдельными и не могут быть перенесены в файл профиля.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileExportDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="55"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="182"/>
        <source>Export Profile</source>
        <translation>Экспорт профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="73"/>
        <source>&quot;%1&quot; is written into a single file you can copy to another computer and import there.</source>
        <translation>&quot;%1&quot; записан в один файл, который можно скопировать на другой компьютер и импортировать туда.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="81"/>
        <source>— choose a file —</source>
        <translation>— выберите файл —</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="83"/>
        <source>Choose...</source>
        <translation>Выбрать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="84"/>
        <source>Save to</source>
        <translation>Сохранить в</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="89"/>
        <source>What goes in</source>
        <translation>Что включается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="96"/>
        <source>Include my workspace files (%1)</source>
        <translation>Включить мои рабочие файлы (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="108"/>
        <source>Protection</source>
        <translation>Защита</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="112"/>
        <source>Protect with a passphrase</source>
        <translation>Защитить парольной фразой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="121"/>
        <source>Passphrase</source>
        <translation>Парольная фраза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="122"/>
        <source>Repeat</source>
        <translation>Повторите</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="125"/>
        <source>No protection</source>
        <translation>Без защиты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="140"/>
        <source>Export</source>
        <translation>Экспорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="164"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="165"/>
        <source>Saved state, key groups and categories</source>
        <translation>Сохранённое состояние, группы ключей и категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="167"/>
        <source>Keys stored inside this profile</source>
        <translation>Ключи, хранящиеся внутри этого профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="170"/>
        <source>Logs and modules are never included. Keys kept outside this profile, such as the system GnuPG keyring, stay where they are.</source>
        <translation>Журналы и модули никогда не включаются. Ключи, хранящиеся вне этого профиля, например системная связка ключей GnuPG, остаются на своих местах.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="205"/>
        <source>The file cannot be opened without this passphrase, and there is no way to recover it. Keys wrapped by this computer&apos;s keychain are not used: the file has to open on another computer.</source>
        <translation>Файл невозможно открыть без этой парольной фразы, и восстановить её невозможно. Ключи, обёрнутые связкой ключей этого компьютера, не используются: файл должен быть открыт на другом компьютере.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="211"/>
        <source>The two entries do not match.</source>
        <translation>Две записи не совпадают.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="217"/>
        <source>Anyone who gets this file can read your keys and everything in the profile, and can change it before you import it.</source>
        <translation>Любой, кто получит этот файл, сможет прочитать ваши ключи и всё содержимое профиля, а также изменить его до того, как вы его импортируете.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileManagerDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="75"/>
        <source>Profiles On This Computer</source>
        <translation>Профили на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="92"/>
        <source>The profiles kept on this computer. Each one has its own settings, keys and saved state. Opening one starts a new window; this window stays exactly as it is.</source>
        <translation>Профили, хранящиеся на этом компьютере. Каждый имеет свои настройки, ключи и сохранённое состояние. Открытие профиля запускает новое окно; это окно остаётся в точности таким, как есть.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="110"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="110"/>
        <source>Folder</source>
        <translation>Папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="110"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="110"/>
        <source>Last Opened</source>
        <translation>Последнее открытие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="110"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="120"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="122"/>
        <source>New...</source>
        <translation>Создать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="123"/>
        <source>Import...</source>
        <translation>Импортировать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="124"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="125"/>
        <source>Open Folder</source>
        <translation>Открыть папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="127"/>
        <source>Open the selected profile in a new window</source>
        <translation>Открыть выбранный профиль в новом окне</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="128"/>
        <source>Start an empty profile on this computer</source>
        <translation>Создать пустой профиль на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="133"/>
        <source>Copy a profile file into a new profile kept on this computer. The file is not used again afterwards.</source>
        <translation>Скопировать файл профиля в новый профиль на этом компьютере. Файл больше не используется.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="136"/>
        <source>Remove the selected profile and everything in it, permanently</source>
        <translation>Удалить выбранный профиль и всё его содержимое навсегда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="138"/>
        <source>Show the selected profile&apos;s folder in the file manager</source>
        <translation>Показать папку выбранного профиля в файловом менеджере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="147"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="181"/>
        <source>%1  (open)</source>
        <translation>%1  (открыт)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="201"/>
        <source>This window</source>
        <translation>Это окно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="203"/>
        <source>Not created yet</source>
        <translation>Ещё не создан</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="206"/>
        <source>Open in another window</source>
        <translation>Открыть в другом окне</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="245"/>
        <source>Cannot Open Profile</source>
        <translation>Не удаётся открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="261"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="270"/>
        <source>Delete Profile</source>
        <translation>Удалить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="262"/>
        <source>Delete &quot;%1&quot;?</source>
        <translation>Удалить &quot;%1&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="263"/>
        <source>Its keys, settings and saved state are removed from this computer permanently.</source>
        <translation>Его ключи, настройки и сохранённое состояние будут безвозвратно удалены с этого компьютера.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="271"/>
        <source>This cannot be undone. Anything stored only in &quot;%1&quot; will be lost.</source>
        <translation>Это действие нельзя отменить. Всё, что хранится только в &quot;%1&quot;, будет потеряно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="280"/>
        <source>Profile Is Open</source>
        <translation>Профиль открыт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="281"/>
        <source>&quot;%1&quot; is open in another window. Close it first.</source>
        <translation>&quot;%1&quot; открыт в другом окне. Сначала закройте его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="299"/>
        <source>Cannot Delete Profile</source>
        <translation>Невозможно удалить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="300"/>
        <source>The profile folder could not be removed:</source>
        <translation>Не удалось удалить папку профиля:</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::QuitDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="35"/>
        <source>Unsaved Files</source>
        <translation>Несохранённые файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="82"/>
        <source>%1 files contain unsaved information.&lt;br/&gt;Save the changes before closing?</source>
        <translation>%1 файл(ов) содержат несохранённые данные.&lt;br/&gt;Сохранить изменения перед закрытием?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="96"/>
        <source>Check the files you want to save:</source>
        <translation>Отметьте файлы, которые хотите сохранить:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="98"/>
        <source>Note</source>
        <translation>Примечание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/QuitDialog.cpp" line="99"/>
        <source>If you don&apos;t save these files, all changes are lost.</source>
        <translation>Если вы не сохраните эти файлы, все изменения будут потеряны.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::RevocationOptionsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/RevocationOptionsDialog.cpp" line="42"/>
        <source>Revocation Reason (Code)</source>
        <translation>Причина отзыва (код)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/RevocationOptionsDialog.cpp" line="43"/>
        <source>Revocation Reason (Text)</source>
        <translation>Причина отзыва (текст)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/RevocationOptionsDialog.cpp" line="44"/>
        <source>Revocation Options</source>
        <translation>Параметры отзыва</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::RpgpEngineTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="697"/>
        <source>Unknown</source>
        <translation>Неизвестно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="706"/>
        <source>GpgFrontend supports multiple OpenPGP backends. Alongside GnuPG, it can use a Rust-based engine (rPGP), giving you the freedom to choose the backend that best fits your needs. The details below describe the rPGP engine compiled into this build.</source>
        <translation>GpgFrontend поддерживает несколько OpenPGP-бэкендов. Помимо GnuPG, он может использовать основанный на Rust движок (rPGP), что позволяет выбрать наиболее подходящий вариант. Ниже приведены сведения о движке rPGP, включённом в данную сборку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="720"/>
        <source>Engine Version:</source>
        <translation>Версия движка:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="723"/>
        <source>Rust Compiler:</source>
        <translation>Компилятор Rust:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="726"/>
        <source>Target:</source>
        <translation>Целевая платформа:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="730"/>
        <source>Build Profile:</source>
        <translation>Профиль сборки:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="734"/>
        <source>rPGP Engine</source>
        <translation>Движок rPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="748"/>
        <source>Key Dependencies</source>
        <translation>Основные зависимости</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="767"/>
        <source>Copy Engine Information</source>
        <translation>Скопировать сведения о движке</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::RpgpTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="42"/>
        <source>Password Cache</source>
        <translation>Кэш паролей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="45"/>
        <source>Password Cache TTL (minutes):</source>
        <translation>TTL кэша паролей (мин):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="47"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="54"/>
        <source> min</source>
        <translation>мин</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="49"/>
        <source>Idle time the rPGP engine keeps an entered passphrase cached. The window is renewed each time the passphrase is used.</source>
        <translation>Время бездействия, в течение которого движок rPGP хранит введённую парольную фразу в кэше. Окно обновляется каждый раз при использовании парольной фразы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="52"/>
        <source>Password Cache Max TTL (minutes):</source>
        <translation>Макс. TTL кэша паролей (мин):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="56"/>
        <source>Absolute lifetime of a cached passphrase, measured from when it was first entered, regardless of use. Never shorter than the TTL.</source>
        <translation>Абсолютное время жизни кэшированной парольной фразы, измеряемое с момента её первого ввода, независимо от использования. Не может быть короче TTL.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="60"/>
        <source>These options only apply to the rPGP engine&apos;s in-memory passphrase cache.</source>
        <translation>Эти параметры применяются только к кэшу парольных фраз в памяти движка rPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="63"/>
        <source>Passphrase Prompt</source>
        <translation>Запрос парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="67"/>
        <source>Passphrase Prompt Timeout (seconds):</source>
        <translation>Тайм-аут запроса парольной фразы (сек):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="69"/>
        <source> s</source>
        <translation>с</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="70"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="72"/>
        <source>Time the passphrase prompt waits for an answer before it closes itself and cancels the operation. Set to 0 to let it wait indefinitely.</source>
        <translation>Время ожидания ответа на запрос парольной фразы, после которого он закрывается и отменяет операцию. Установите 0, чтобы ждать бесконечно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="77"/>
        <source>Applies to the passphrase prompt GpgFrontend shows for the rPGP engine. GnuPG keys are unlocked through pinentry, which has its own timeout.</source>
        <translation>Применяется к запросу парольной фразы, который GpgFrontend показывает для движка rPGP. Ключи GnuPG разблокируются через pinentry, у которого свой тайм-аут.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SetOwnerTrustLevel</name>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="101"/>
        <source>Undefined</source>
        <translation>Не определено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="99"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="97"/>
        <source>Marginal</source>
        <translation>Частичное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="95"/>
        <source>Full</source>
        <translation>Полное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="48"/>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="93"/>
        <source>Ultimate</source>
        <translation>Абсолютное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="87"/>
        <source>Modify Owner Trust Level</source>
        <translation>Изменить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="88"/>
        <source>Trust for the Key Pair:</source>
        <translation>Доверие для пары ключей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="113"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="114"/>
        <source>Modify Owner Trust Level failed.</source>
        <translation>Не удалось изменить уровень доверия владельца.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SettingsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="92"/>
        <source>General</source>
        <translation>Общие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="93"/>
        <source>Appearance</source>
        <translation>Внешний вид</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="122"/>
        <source>Network</source>
        <translation>Сеть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="94"/>
        <source>Key Databases</source>
        <translation>Базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="85"/>
        <source>Application</source>
        <translation>Приложение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="86"/>
        <source>Keys &amp; Engines</source>
        <translation>Ключи и движки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="87"/>
        <source>Features</source>
        <translation>Функции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="88"/>
        <source>System</source>
        <translation>Система</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="95"/>
        <source>GnuPG</source>
        <translation>GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="96"/>
        <source>Advanced</source>
        <translation>Расширенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>startup</source>
        <translation>Запуск</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>confirm import</source>
        <translation>Подтверждать импорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="108"/>
        <source>locale</source>
        <translation>Локаль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="109"/>
        <source>translation</source>
        <translation>Перевод</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="109"/>
        <source>data</source>
        <translation>Данные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="109"/>
        <source>cache</source>
        <translation>Кэш</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>theme</source>
        <translation>Тема</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>icon</source>
        <translation>Значок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>font size</source>
        <translation>Размер шрифта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="113"/>
        <source>font family</source>
        <translation>Гарнитура шрифта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="114"/>
        <source>toolbar</source>
        <translation>Панель инструментов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="114"/>
        <source>actions</source>
        <translation>Действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="114"/>
        <source>instant messaging</source>
        <translation>Мгновенные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>text editor</source>
        <translation>Текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>status panel</source>
        <translation>Панель состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>proxy</source>
        <translation>Прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>socks</source>
        <translation>SOCKS</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>http</source>
        <translation>HTTP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>timeout</source>
        <translation>тайм-аут</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="125"/>
        <source>connection</source>
        <translation>соединение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="131"/>
        <source>keyring</source>
        <translation>связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="131"/>
        <source>gpg home</source>
        <translation>домашний каталог GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="131"/>
        <source>database path</source>
        <translation>путь к базе данных</translation>
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
        <translation>путь к исполняемому файлу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="138"/>
        <source>custom install</source>
        <translation>пользовательская установка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>password cache</source>
        <translation>кэш паролей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>reload</source>
        <translation>перезагрузить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>restart</source>
        <translation>перезапустить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="139"/>
        <source>gpg-agent</source>
        <translation>gpg-agent</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="140"/>
        <source>maintenance</source>
        <translation>обслуживание</translation>
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
        <translation>движок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="150"/>
        <source>Instant Messaging</source>
        <translation>Мгновенные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>message book</source>
        <translation>Книга сообщений</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>phrase</source>
        <translation>Фраза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="152"/>
        <source>token</source>
        <translation>Токен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="156"/>
        <source>security level</source>
        <translation>Уровень безопасности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="156"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="156"/>
        <source>keychain</source>
        <translation>Связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="157"/>
        <source>log level</source>
        <translation>Уровень логирования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="157"/>
        <source>ring buffer</source>
        <translation>Кольцевой буфер</translation>
    </message>
    <message>
        <source>integrity check</source>
        <translation type="vanished">Проверка целостности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="157"/>
        <source>ENV.ini</source>
        <translation>ENV.ini</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="180"/>
        <source>Search settings…</source>
        <translation>Поиск настроек…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="233"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="468"/>
        <source>Restart Required</source>
        <translation>Требуется перезапуск</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="469"/>
        <source>Some of your changes only take effect after GpgFrontend restarts.</source>
        <translation>Некоторые изменения вступают в силу только после перезапуска GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="471"/>
        <source>Some of your changes only take effect after the interface reloads.</source>
        <translation>Некоторые изменения вступают в силу только после перезагрузки интерфейса.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="474"/>
        <source>Changes needing this were made on: %1.

Choose Cancel to discard everything you changed in this dialog and keep the current settings.</source>
        <translation>Изменения, требующие этого, были сделаны: %1.

Нажмите «Отмена», чтобы отменить все изменения в этом диалоге и сохранить текущие настройки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="479"/>
        <source>Save and Restart</source>
        <translation>Сохранить и перезапустить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="479"/>
        <source>Save and Reload</source>
        <translation>Сохранить и перезагрузить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="570"/>
        <source>System Default</source>
        <translation>По умолчанию в системе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="223"/>
        <source>Preference</source>
        <translation>Настройки</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SigningKeysPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="81"/>
        <source>Select Signing Key(s)</source>
        <translation>Выберите ключ(и) подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="86"/>
        <source>Choose Signing Key(s) or Subkey(s)</source>
        <translation>Выберите ключ(и) подписи или подключ(и)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="92"/>
        <source>Select the private key(s) or specific signing subkey(s) to use for this operation. Expand a key to choose a particular subkey.</source>
        <translation>Выберите закрытый ключ(и) или конкретный подключ(и) подписи для этой операции. Разверните ключ, чтобы выбрать нужный подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="106"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="109"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="114"/>
        <source>No Key Selected</source>
        <translation>Ключ не выбран</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SigningKeysPicker.cpp" line="115"/>
        <source>Please select at least one signing key or subkey.</source>
        <translation>Пожалуйста, выберите хотя бы один ключ подписи или подключ.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SmartCardControllerDialog</name>
    <message>
        <source>Smart Card(s):</source>
        <translation type="vanished">Смарт-карта(ы):</translation>
    </message>
    <message>
        <source>Key Stub(s) in Key Database(s):</source>
        <translation type="vanished">Заглушка(и) ключей в базе(ах) данных ключей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="156"/>
        <source>Change Name</source>
        <translation>Изменить имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="158"/>
        <source>Change Language</source>
        <translation>Изменить язык</translation>
    </message>
    <message>
        <source>Change Gender</source>
        <translation type="vanished">Изменить пол</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="162"/>
        <source>Change Login Data</source>
        <translation>Изменить данные для входа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="164"/>
        <source>Change Public Key URL</source>
        <translation>Изменить URL открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="169"/>
        <source>Change PIN</source>
        <translation>Изменить PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="171"/>
        <source>Change Admin PIN</source>
        <translation>Изменить PIN администратора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="173"/>
        <source>Change Reset Code</source>
        <translation>Изменить код сброса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="139"/>
        <source>Fetch</source>
        <translation>Получить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="116"/>
        <source>Slot</source>
        <translation>Слот</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="125"/>
        <source>Key Info (ki)</source>
        <translation>Информация о ключе (ki)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="126"/>
        <source>Additional Auth (aac)</source>
        <translation>Дополнительная аутентификация (aac)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="127"/>
        <source>Biometric Terminal (bt)</source>
        <translation>Биометрический терминал (bt)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="128"/>
        <source>KDF Supported</source>
        <translation>Поддержка KDF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="178"/>
        <source>Restart All Gpg-Agents</source>
        <translation>Перезапустить все агенты GPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="137"/>
        <source>Generate Card Keys</source>
        <translation>Сгенерировать ключи карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="140"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <source>Operations</source>
        <translation type="vanished">Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="192"/>
        <source>Successful Operation</source>
        <translation>Успешная операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="193"/>
        <source>Restart all the GnuPG&apos;s components successfully</source>
        <translation>Все компоненты GnuPG успешно перезапущены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="196"/>
        <source>Failed Operation</source>
        <translation>Неудавшаяся операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="197"/>
        <source>Failed to restart all or one of the GnuPG&apos;s component(s)</source>
        <translation>Не удалось перезапустить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="227"/>
        <source>Generate card key failed.</source>
        <translation>Не удалось сгенерировать ключ карты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="95"/>
        <source>Smart Card Controller</source>
        <translation>Управление смарт-картами</translation>
    </message>
    <message>
        <source>OpenPGP Card Information</source>
        <translation type="vanished">Информация о карте OpenPGP</translation>
    </message>
    <message>
        <source>Basic Information</source>
        <translation type="vanished">Основная информация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="100"/>
        <source>Reader</source>
        <translation>Считыватель</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="101"/>
        <source>Serial Number</source>
        <translation>Серийный номер</translation>
    </message>
    <message>
        <source>Card Type</source>
        <translation type="vanished">Тип карты</translation>
    </message>
    <message>
        <source>Card Version</source>
        <translation type="vanished">Версия карты</translation>
    </message>
    <message>
        <source>App Type</source>
        <translation type="vanished">Тип приложения</translation>
    </message>
    <message>
        <source>App Version</source>
        <translation type="vanished">Версия приложения</translation>
    </message>
    <message>
        <source>Manufacturer ID</source>
        <translation type="vanished">Идентификатор производителя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="102"/>
        <source>Manufacturer</source>
        <translation>Производитель</translation>
    </message>
    <message>
        <source>Card Holder</source>
        <translation type="vanished">Держатель карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="105"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="106"/>
        <source>Sex</source>
        <translation>Пол</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="109"/>
        <source>Signature Counter</source>
        <translation>Счётчик подписей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="110"/>
        <source>CHV1 Cached</source>
        <translation>CHV1 в кэше</translation>
    </message>
    <message>
        <source>CHV Max Length</source>
        <translation type="vanished">Максимальная длина CHV</translation>
    </message>
    <message>
        <source>CHV Retry Left</source>
        <translation type="vanished">Осталось попыток CHV</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="111"/>
        <source>KDF Status</source>
        <translation>Статус KDF</translation>
    </message>
    <message>
        <source>UIF</source>
        <translation type="vanished">UIF</translation>
    </message>
    <message>
        <source>Sign</source>
        <translation type="vanished">Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="348"/>
        <source>Enabled</source>
        <translation>Включено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="349"/>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
    <message>
        <source>Encrypt</source>
        <translation type="vanished">Зашифровать</translation>
    </message>
    <message>
        <source>Authenticate</source>
        <translation type="vanished">Аутентификация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="129"/>
        <source>Status Indicator</source>
        <translation>Индикатор состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="133"/>
        <source>No card detected</source>
        <translation>Карта не обнаружена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="135"/>
        <source>Cardholder</source>
        <translation>Держатель карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="136"/>
        <source>Access Codes</source>
        <translation>Коды доступа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="138"/>
        <source>Move Key to Card</source>
        <translation>Переместить ключ на карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="141"/>
        <source>More Actions</source>
        <translation>Ещё действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="160"/>
        <source>Change Sex</source>
        <translation>Изменить пол</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="201"/>
        <source>Open GnuPG Smart Card HOWTO</source>
        <translation>Открыть руководство GnuPG по смарт-картам</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="296"/>
        <source>Unnamed Card</source>
        <translation>Безымянная карта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="310"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="312"/>
        <source>%1, version %2</source>
        <translation>%1, версия %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="315"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="316"/>
        <source>Not set</source>
        <translation>Не задано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="319"/>
        <source>● Ready</source>
        <translation>● Готов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="331"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="331"/>
        <source>Reset Code</source>
        <translation>Код сброса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="332"/>
        <source>Admin PIN</source>
        <translation>PIN администратора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="338"/>
        <source>%1 n/a</source>
        <translation>%1 н/д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="342"/>
        <source>%1 %2 left</source>
        <translation>Осталось %1 %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="345"/>
        <source>%1 retries left, maximum length %2</source>
        <translation>Осталось %1 попыток, макс. длина %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="351"/>
        <source>User Interaction Flag — Sign: %1 · Encrypt: %2 · Authenticate: %3</source>
        <translation>Флаг взаимодействия с пользователем — Подпись: %1 · Шифрование: %2 · Аутентификация: %3</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="396"/>
        <source>N/A</source>
        <translation>Н/Д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="472"/>
        <source>Read the GnuPG Smart Card HOWTO</source>
        <translation>Прочитать руководство GnuPG по смарт-картам</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="558"/>
        <source>No Card</source>
        <translation>Нет карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="559"/>
        <source>No smart card is currently selected.</source>
        <translation>В настоящее время смарт-карта не выбрана.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="566"/>
        <source>Not Supported</source>
        <translation>Не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="567"/>
        <source>Moving a key to a smart card is only supported for GnuPG key databases. The rpgp engine is not supported for this feature.</source>
        <translation>Перемещение ключа на смарт-карту поддерживается только для баз данных ключей GnuPG. Движок rpgp не поддерживается для этой функции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="589"/>
        <source>Cardholder&apos;s Surname</source>
        <translation>Фамилия держателя карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="590"/>
        <source>Please enter your surname (e.g., Lee):</source>
        <translation>Введите вашу фамилию (например, Иванов):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="595"/>
        <source>Cardholder&apos;s Given Name</source>
        <translation>Имя держателя карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="596"/>
        <source>Please enter your given name (e.g., Chris):</source>
        <translation>Введите ваше имя (например, Иван):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="605"/>
        <source>Too Long</source>
        <translation>Слишком длинное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="606"/>
        <source>Combined name too long (max 39 characters).</source>
        <translation>Комбинированное имя слишком длинное (максимум 39 символов).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="621"/>
        <source>Male</source>
        <translation>Мужской</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="621"/>
        <source>Female</source>
        <translation>Женский</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="625"/>
        <source>Select sex to store in &apos;%1&apos;</source>
        <translation>Выберите пол для сохранения в «%1»</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="640"/>
        <source>Enter new value for attribute &apos;%1&apos;</source>
        <translation>Введите новое значение для атрибута «%1»</translation>
    </message>
    <message>
        <source>Status</source>
        <translation type="vanished">Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="362"/>
        <source>Not enabled</source>
        <translation>Не включено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="365"/>
        <source>Enabled (no protection)</source>
        <translation>Включено (без защиты)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="368"/>
        <source>Enabled with salt protection</source>
        <translation>Включено с защитой солью</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="303"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="371"/>
        <source>Unknown</source>
        <translation>Неизвестно</translation>
    </message>
    <message>
        <source>Key Information</source>
        <translation type="vanished">Информация о ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="382"/>
        <source>No key information available.</source>
        <translation>Информация о ключе недоступна.</translation>
    </message>
    <message>
        <source>No.</source>
        <translation type="vanished">№</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="117"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="116"/>
        <source>Created</source>
        <translation>Создано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="117"/>
        <source>Grip</source>
        <translation>Дескриптор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="116"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="97"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="103"/>
        <source>Card</source>
        <translation>Карта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="99"/>
        <source>Identity</source>
        <translation>Личность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="104"/>
        <source>Application</source>
        <translation>Приложение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="108"/>
        <source>Access &amp; Status</source>
        <translation>Доступ и статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="113"/>
        <source>Keys on Card</source>
        <translation>Ключи на карте</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="116"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="116"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <source>Curve</source>
        <translation type="vanished">Кривая</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="124"/>
        <source>Extended Capabilities</source>
        <translation>Расширенные возможности</translation>
    </message>
    <message>
        <source>Key Info (ki): %1</source>
        <translation type="vanished">Информация о ключе (ki): %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="357"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="411"/>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="357"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="412"/>
        <source>No</source>
        <translation>Нет</translation>
    </message>
    <message>
        <source>Additional Auth (aac): %1</source>
        <translation type="vanished">Дополнительная аутентификация (aac): %1</translation>
    </message>
    <message>
        <source>Biometric Terminal (bt): %1</source>
        <translation type="vanished">Биометрический терминал (bt): %1</translation>
    </message>
    <message>
        <source>KDF Supported: %1</source>
        <translation type="vanished">Поддержка KDF: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="131"/>
        <source>Additional Info</source>
        <translation>Дополнительная информация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="454"/>
        <source>No OpenPGP Smart Card Found</source>
        <translation>Смарт-карта OpenPGP не найдена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="459"/>
        <source>No OpenPGP-compatible smart card has been detected.</source>
        <translation>Совместимая с OpenPGP смарт-карта не обнаружена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="460"/>
        <source>An OpenPGP Smart Card is a physical device that securely stores your private cryptographic keys and can be used for digital signing, encryption, and authentication. Popular examples include YubiKey, Nitrokey, and other GnuPG-compatible tokens.</source>
        <translation>Смарт-карта OpenPGP — это физическое устройство для безопасного хранения закрытых криптографических ключей, которое может использоваться для цифровой подписи, шифрования и аутентификации. Примеры: YubiKey, Nitrokey и другие совместимые с GnuPG токены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="466"/>
        <source>Make sure your card is inserted and properly recognized by the system. You can also try reconnecting the card or restarting the application.</source>
        <translation>Убедитесь, что карта вставлена и правильно распознана системой. Также попробуйте переподключить карту или перезапустить приложение.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="481"/>
        <source>Note: Smart card support of GpgFrontend requires GnuPG version 2.3.0 or later.</source>
        <translation>Примечание: поддержка смарт-карт в GpgFrontend требует GnuPG версии 2.3.0 или выше.</translation>
    </message>
    <message>
        <source>Read the GnuPG Smart Card HOWTO: </source>
        <translation type="vanished">Читайте руководство по смарт-картам GnuPG: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="624"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="639"/>
        <source>Modify Card Attribute</source>
        <translation>Изменить атрибут карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="658"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="672"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="659"/>
        <source>Attribute operation completed successfully.</source>
        <translation>Операция с атрибутом успешно выполнена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="226"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="673"/>
        <source>PIN operation completed successfully.</source>
        <translation>Операция с PIN успешно выполнена.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::StatusIndicatorInfo</name>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="56"/>
        <source>Click to manage profiles.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="61"/>
        <source>Click to see the full status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="72"/>
        <source>Profile</source>
        <translation type="unfinished">Профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="78"/>
        <source>%1 (temporary)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="84"/>
        <source>Opened from a file, and not kept on this computer. Closing asks whether to save the changes back into it.</source>
        <translation type="unfinished">Открыт из файла и не хранится на этом компьютере. При закрытии будет предложено сохранить изменения обратно в него.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="88"/>
        <source>This window&apos;s profile — its own settings, keys and saved state</source>
        <translation type="unfinished">Профиль этого окна — его собственные настройки, ключи и сохранённое состояние</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="111"/>
        <source>Current OpenPGP backend and version</source>
        <translation type="unfinished">Текущий бэкенд OpenPGP и его версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="115"/>
        <source>Key database: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="132"/>
        <source>Portable Mode</source>
        <translation type="unfinished">Портативный режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="134"/>
        <source>Installed Mode</source>
        <translation type="unfinished">Режим установки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="139"/>
        <source>Running from the folder it was unpacked into, taking its settings and keys along with it.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="143"/>
        <source>Installed on this computer, with its settings and keys kept in this user&apos;s data folder.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="147"/>
        <source>This profile keeps its own keys, separate from the rest of the computer.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::StatusTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="592"/>
        <source>Application Status</source>
        <translation>Статус приложения</translation>
    </message>
    <message>
        <source>Default</source>
        <translation type="vanished">По умолчанию</translation>
    </message>
    <message>
        <source>Standard</source>
        <translation type="vanished">Стандартный</translation>
    </message>
    <message>
        <source>Enhanced</source>
        <translation type="vanished">Улучшенный</translation>
    </message>
    <message>
        <source>High</source>
        <translation type="vanished">Высокий</translation>
    </message>
    <message>
        <source>Unknown</source>
        <translation type="vanished">Неизвестно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="540"/>
        <source>Portable Mode</source>
        <translation>Портативный режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="540"/>
        <source>Installed Mode</source>
        <translation>Режим установки</translation>
    </message>
    <message>
        <source>Self-Check Active</source>
        <translation type="vanished">Самопроверка активна</translation>
    </message>
    <message>
        <source>Self-Check Disabled</source>
        <translation type="vanished">Самопроверка отключена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="543"/>
        <source>Active</source>
        <translation>Активен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="543"/>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="546"/>
        <source>Default Pinentry Program</source>
        <translation>Программа ввода PIN по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="553"/>
        <source>Secure Level:</source>
        <translation>Уровень защиты:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="555"/>
        <source>Application Key Protection:</source>
        <translation>Защита ключей приложения:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="564"/>
        <source>System Credential Store:</source>
        <translation>Системное хранилище учетных данных:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="566"/>
        <source>Unavailable</source>
        <translation>Недоступно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="576"/>
        <source>Credential Store Detail:</source>
        <translation>Детали хранилища учетных данных:</translation>
    </message>
    <message>
        <source>Security Level:</source>
        <translation type="vanished">Уровень безопасности:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="580"/>
        <source>Running Mode:</source>
        <translation>Режим запуска:</translation>
    </message>
    <message>
        <source>Self-Check Status:</source>
        <translation type="vanished">Статус самопроверки:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="584"/>
        <source>GnuPG Offline Mode:</source>
        <translation>Автономный режим GnuPG:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="587"/>
        <source>Pinentry Program Path:</source>
        <translation>Путь к программе ввода PIN:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="605"/>
        <source>Profile:</source>
        <translation>Профиль:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="608"/>
        <source>Profile Type:</source>
        <translation>Тип профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="610"/>
        <source>Profile Folder:</source>
        <translation>Папка профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="617"/>
        <source>Keys:</source>
        <translation>Ключи:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="619"/>
        <source>Inside this profile</source>
        <translation>Внутри этого профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="620"/>
        <source>System keyring</source>
        <translation>Системная связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="625"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="629"/>
        <source>Workspace:</source>
        <translation>Рабочая область:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="626"/>
        <source>None</source>
        <translation>Нет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="636"/>
        <source>Profile Layout Version:</source>
        <translation>Версия макета профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="644"/>
        <source>Imported From Package:</source>
        <translation>Импортировано из пакета:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="654"/>
        <source>Profiles Folder:</source>
        <translation>Папка профилей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="659"/>
        <source>Profile</source>
        <translation>Профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="676"/>
        <source>Supported OpenPGP Engines</source>
        <translation>Поддерживаемые движки OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="680"/>
        <source>Tip: These values reflect the current startup environment and may help when reporting issues.</source>
        <translation>Подсказка: Эти значения отражают текущую среду запуска и могут помочь при сообщении о проблемах.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SubkeyGenerateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="150"/>
        <source>Tipps: if the key pair has a passphrase, the subkey&apos;s passphrase must be equal to it.</source>
        <translation>Подсказка: если у пары ключей есть парольная фраза, парольная фраза подключа должна совпадать с ней.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="166"/>
        <source>Generate New Subkey</source>
        <translation>Сгенерировать новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="126"/>
        <source>Authentication</source>
        <translation>Аутентификация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="120"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="121"/>
        <source>Key Length</source>
        <translation>Длина ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="122"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="123"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="124"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="125"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="127"/>
        <source>Non Expired</source>
        <translation>Действующий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="128"/>
        <source>No Passphrase</source>
        <translation>Без парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="129"/>
        <source>Second Algorithm</source>
        <translation>Алгоритм подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="130"/>
        <source>Second Key Length</source>
        <translation>Длина подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="161"/>
        <source>Note: post-quantum (PQC) subkey algorithms are unavailable here because the primary key uses the v4 key format. Generate a v6 key to use PQC algorithms.</source>
        <translation>Примечание: постквантовые (PQC) алгоритмы подключа недоступны, так как первичный ключ использует формат ключа v4. Сгенерируйте ключ v6 для использования алгоритмов PQC.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="474"/>
        <source>Please give a valid subkey algorithm.</source>
        <translation>Укажите допустимый алгоритм подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="484"/>
        <source>Please give a valid second algorithm.</source>
        <translation>Укажите допустимый алгоритм подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="494"/>
        <source>Time to subkey expiration must not be less than 120 seconds.</source>
        <translation>Время до истечения подключа должно быть не менее 120 секунд.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="511"/>
        <source>Generating</source>
        <translation>Генерация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="524"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="525"/>
        <source>Unknown error occurred</source>
        <translation>Произошла неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="553"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="554"/>
        <source>Subkey generation completed successfully.</source>
        <translation>Генерация подключа выполнена успешно.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TextEdit</name>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="197"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="515"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="198"/>
        <source>Cannot read file %1:
%2.</source>
        <translation>Невозможно прочитать файл %1:
%2.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="116"/>
        <source>Open file</source>
        <translation>Открыть файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="93"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="101"/>
        <source>Open File</source>
        <translation>Открыть файл</translation>
    </message>
    <message>
        <source>Error</source>
        <translation type="vanished">Ошибка</translation>
    </message>
    <message>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation type="vanished">Невозможно открыть этот файл. Убедитесь, что это обычный файл и он доступен для чтения.</translation>
    </message>
    <message>
        <source>Cannot open this file. The file is TOO LARGE (&gt;1MB) for GpgFrontend Text Editor.</source>
        <translation type="vanished">Невозможно открыть этот файл. Файл СЛИШКОМ БОЛЬШОЙ (&gt;1 МБ) для текстового редактора GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="474"/>
        <source>File Open Error</source>
        <translation>Ошибка открытия файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="475"/>
        <source>The file &quot;%1&quot; could not be opened.</source>
        <translation>Не удалось открыть файл «%1».</translation>
    </message>
    <message>
        <source>Binary File Detected</source>
        <translation type="vanished">Обнаружен двоичный файл</translation>
    </message>
    <message>
        <source>The file &quot;%1&quot; appears to be a binary file and will not be opened.</source>
        <translation type="vanished">Файл «%1» является двоичным файлом и не будет открыт.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="135"/>
        <source>Unknown Tab Type</source>
        <translation>Неизвестный тип вкладки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="136"/>
        <source>The current tab has an unknown type. Cannot save.</source>
        <translation>Текущая вкладка имеет неизвестный тип. Сохранение невозможно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="155"/>
        <source>Unsupported Operation</source>
        <translation>Неподдерживаемая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="156"/>
        <source>The save file operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция сохранения файла для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="216"/>
        <source>Save file</source>
        <translation>Сохранить файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="276"/>
        <source>Unsaved document</source>
        <translation>Несохранённый документ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="277"/>
        <source>The document &quot;%1&quot; has been modified. Do you want to save your changes?</source>
        <translation>Документ «%1» был изменён. Сохранить изменения?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="280"/>
        <source>Note:</source>
        <translation>Примечание:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="281"/>
        <source>If you don&apos;t save these files, all changes are lost.</source>
        <translation>Если вы не сохраните эти файлы, все изменения будут потеряны.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="515"/>
        <source>No document to print</source>
        <translation>Нет документа для печати</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TextEditTabWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="331"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="354"/>
        <source>File Open Error</source>
        <translation>Ошибка открытия файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="277"/>
        <source>The file &quot;%1&quot; could not be opened.</source>
        <translation>Не удалось открыть файл «%1».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="150"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="151"/>
        <source>Copy Path</source>
        <translation>Копировать путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="152"/>
        <source>Reveal in File Browser</source>
        <translation>Показать в файловом менеджере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="257"/>
        <source>The file does not exist.</source>
        <translation>Файл не существует.</translation>
    </message>
    <message>
        <source>The file &quot;%1&quot; is larger than 1 MB and will not be opened in the text editor.</source>
        <translation type="vanished">Файл «%1» превышает 1 МБ и не будет открыт в текстовом редакторе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="266"/>
        <source>The file &quot;%1&quot; is larger than 4 MB and will not be opened in the text editor.</source>
        <translation>Файл «%1» превышает 4 МБ и не будет открыт в текстовом редакторе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="285"/>
        <source>The file &quot;%1&quot; appears to be a binary file and will not be opened.</source>
        <translation>Файл «%1» является двоичным файлом и не будет открыт.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="332"/>
        <source>The file &quot;%1&quot; does not exist.</source>
        <translation>Файл «%1» не существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="489"/>
        <source>Select Default Path</source>
        <translation>Выбрать путь по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="554"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="984"/>
        <source>Workspace</source>
        <translation>Рабочая область</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="956"/>
        <source>Directory Permission Denied</source>
        <translation>Доступ к каталогу запрещён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="957"/>
        <source>You do not have permission to access the directory &quot;%1&quot;.</source>
        <translation>У вас нет прав доступа к каталогу «%1».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="979"/>
        <source>Root</source>
        <translation>Корень</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TranslatorsTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="496"/>
        <source>Thanks to all translators</source>
        <translation>Спасибо всем переводчикам</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="506"/>
        <source>Translator information is not available.</source>
        <translation>Информация о переводчике недоступна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="510"/>
        <source>If you want to help improve localization, please read the &lt;a href=&apos;https://gpgfrontend.bktus.com/appendix/translate-interface/&apos;&gt;translation guide&lt;/a&gt;.!</source>
        <translation>Если вы хотите помочь улучшить локализацию, пожалуйста, ознакомьтесь с &lt;a href=&apos;https://gpgfrontend.bktus.com/appendix/translate-interface/&apos;&gt;руководством по переводу&lt;/a&gt;.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::WaitingDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/WaitingDialog.cpp" line="46"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/WaitingDialog.cpp" line="84"/>
        <source>Cancelling...</source>
        <translation>Отменяется...</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::Wizard</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="225"/>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="232"/>
        <source>Welcome to GpgFrontend</source>
        <translation>Добро пожаловать в GpgFrontend</translation>
    </message>
</context>
<context>
    <name>GpgKey</name>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="107"/>
        <source>Unknown</source>
        <translation>Неизвестно</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="109"/>
        <source>Undefined</source>
        <translation>Не определено</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="111"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="113"/>
        <source>Marginal</source>
        <translation>Частичное</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="115"/>
        <source>Full</source>
        <translation>Полное</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="103"/>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="117"/>
        <source>Ultimate</source>
        <translation>Абсолютное</translation>
    </message>
</context>
<context>
    <name>QCoreApplication</name>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="153"/>
        <source>File Hash Information</source>
        <translation>Хеш-информация о файле</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="127"/>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="170"/>
        <source>Filename</source>
        <translation>Имя файла</translation>
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
        <translation>Размер файла</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="168"/>
        <source>Error: cannot read target file</source>
        <translation>Ошибка: не удалось прочитать целевой файл</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="202"/>
        <source>target path doesn&apos;t exists</source>
        <translation>целевой путь не существует</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="208"/>
        <source>do NOT have permission to write path</source>
        <translation>нет прав на запись по указанному пути</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="214"/>
        <source>do NOT have permission to read/write file</source>
        <translation>нет прав на чтение/запись файла</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="217"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1622"/>
        <source>Sync All Public Key</source>
        <translation>Синхронизировать все открытые ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1623"/>
        <source>You have not checked any public keys that you want to synchronize, do you want to synchronize all local public keys from the key server?</source>
        <translation>Вы не отметили открытые ключи для синхронизации. Хотите синхронизировать все локальные открытые ключи с сервера ключей?</translation>
    </message>
    <message>
        <source>Cannot Find GpgConf</source>
        <translation type="vanished">Не удалось найти GpgConf</translation>
    </message>
    <message>
        <source>Cannot Find GnuPG</source>
        <translation type="vanished">Не удалось найти GnuPG</translation>
    </message>
    <message>
        <source>Cannot get Infos from GpgConf</source>
        <translation type="vanished">Не удалось получить информацию из GpgConf</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="530"/>
        <source>Basic Path Initiation Failed</source>
        <translation>Не удалось инициализировать базовые пути</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="558"/>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="616"/>
        <source>No Supported OpenPGP Engine Detected</source>
        <translation>Не обнаружено поддерживаемого движка OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="600"/>
        <source>No valid Key Database</source>
        <translation>Нет допустимой базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="648"/>
        <source>GpgME Default Context Initiation Failed</source>
        <translation>Не удалось инициализировать контекст GpgME по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="661"/>
        <source>Gpg Default Key Database Initiation Failed</source>
        <translation>Не удалось инициализировать базу данных ключей GPG по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="99"/>
        <source>Starting GpgFrontend</source>
        <translation>Запуск GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="105"/>
        <source>Loading essential information</source>
        <translation>Загрузка основной информации</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="111"/>
        <source>GpgFrontend is checking your OpenPGP environment and preparing the default engine. This may take a few seconds.</source>
        <translation>GpgFrontend проверяет вашу среду OpenPGP и подготавливает движок по умолчанию. Это может занять несколько секунд.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="116"/>
        <source>Please keep this window open while the initialization is running.</source>
        <translation>Пожалуйста, не закрывайте это окно во время инициализации.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="129"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../../../../src/GpgFrontend.cpp" line="107"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="235"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="259"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="201"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="647"/>
        <source>Cannot Open Profile</source>
        <translation>Не удалось открыть профиль</translation>
    </message>
    <message>
        <source>Program Self-Test Failed</source>
        <translation type="vanished">Самотестирование программы не пройдено</translation>
    </message>
    <message>
        <source>The application has detected an issue while verifying essential libraries and binaries that were digitally signed during the build. This means one or more files may have been altered or are being loaded from the wrong location. For security reasons, the program must now exit.</source>
        <translation type="vanished">Приложение обнаружило проблему при проверке основных библиотек и исполняемых файлов, которые были подписаны при сборке. Это означает, что один или несколько файлов могли быть изменены или загружены из неправильного расположения. По соображениям безопасности программа должна завершить работу.</translation>
    </message>
    <message>
        <source>PIN Required</source>
        <translation type="vanished">Требуется PIN</translation>
    </message>
    <message>
        <source>High security mode is enabled.</source>
        <translation type="vanished">Включён режим высокой безопасности.</translation>
    </message>
    <message>
        <source>To unlock the application please enter your PIN.</source>
        <translation type="vanished">Для разблокировки приложения введите PIN-код.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="304"/>
        <source>Save Key Failed</source>
        <translation>Не удалось сохранить ключ</translation>
    </message>
    <message>
        <source>Failed to save the secure key to disk at: %1</source>
        <translation type="vanished">Не удалось сохранить безопасный ключ на диск: %1</translation>
    </message>
    <message>
        <source>Please check your storage or try running as administrator.</source>
        <translation type="vanished">Проверьте хранилище или попробуйте запустить от имени администратора.</translation>
    </message>
    <message>
        <source>Time Rotation Secure Key Generation Failed</source>
        <translation type="vanished">Не удалось сгенерировать ключ с ротацией по времени</translation>
    </message>
    <message>
        <source>Failed to generate a salt; falling back to less-secure key.</source>
        <translation type="vanished">Не удалось сгенерировать соль; используется менее безопасный ключ.</translation>
    </message>
    <message>
        <source>Failed to derive time-rotated key; falling back to less-secure key.</source>
        <translation type="vanished">Не удалось получить ключ с ротацией по времени; используется менее безопасный ключ.</translation>
    </message>
    <message>
        <source>Encrypt Key Failed</source>
        <translation type="vanished">Не удалось зашифровать ключ</translation>
    </message>
    <message>
        <source>Failed to encrypt the secure key with your PIN. The key will not be saved to disk.</source>
        <translation type="vanished">Не удалось зашифровать безопасный ключ вашим PIN. Ключ не будет сохранён на диск.</translation>
    </message>
    <message>
        <source>Failed to generate a secure application key using OpenSSL. A less secure fallback key will be used. Please check your system&apos;s cryptography support.</source>
        <translation type="vanished">Не удалось сгенерировать безопасный ключ приложения с помощью OpenSSL. Будет использован менее безопасный резервный ключ. Проверьте поддержку криптографии в вашей системе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="339"/>
        <source>Secure Key Generation Failed</source>
        <translation>Не удалось сгенерировать безопасный ключ</translation>
    </message>
    <message>
        <source>Failed to compute key ID.</source>
        <translation type="vanished">Не удалось вычислить идентификатор ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="305"/>
        <source>The application secure key could not be saved: %1</source>
        <translation>Не удалось сохранить безопасный ключ приложения: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="308"/>
        <source>Anything saved now would be unreadable after a restart, so the application will not continue. Please check your storage and permissions.</source>
        <translation>Сохранённое сейчас будет нечитаемо после перезапуска, поэтому приложение не будет продолжено. Проверьте хранилище и разрешения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="318"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="327"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="347"/>
        <source>App Secure Key Error</source>
        <translation>Ошибка безопасного ключа приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="319"/>
        <source>Failed to decrypt the application secure key. The key file may be corrupted.</source>
        <translation>Не удалось расшифровать безопасный ключ приложения. Возможно, файл ключа повреждён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="321"/>
        <source>Please clear the secure key and try again.</source>
        <translation>Пожалуйста, очистите безопасный ключ и повторите попытку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="328"/>
        <source>Failed to read the application secure key from disk at: %1</source>
        <translation>Не удалось прочитать безопасный ключ приложения с диска: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="332"/>
        <source>Please ensure the key file exists and is accessible, or try re-initializing the secure key.</source>
        <translation>Убедитесь, что файл ключа существует и доступен, или попробуйте повторно инициализировать безопасный ключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="340"/>
        <source>Failed to generate an application secure key.</source>
        <translation>Не удалось сгенерировать безопасный ключ приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="341"/>
        <source>Please check your system&apos;s cryptography support.</source>
        <translation>Проверьте поддержку криптографии в вашей системе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="215"/>
        <source>Reset Secure Key</source>
        <translation>Сбросить безопасный ключ</translation>
    </message>
    <message>
        <source>The key file at %1 could not be removed.</source>
        <translation type="vanished">Не удалось удалить файл ключа: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="367"/>
        <source>System Keychain Unavailable</source>
        <translation>Связка ключей системы недоступна</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="368"/>
        <source>The application key could not be protected using the system keychain, so it remains stored unprotected.</source>
        <translation>Не удалось защитить ключ приложения с помощью системной связки ключей, поэтому он остаётся незащищённым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="371"/>
        <source>This setting has been turned off. You can turn it on again once a keychain is available.</source>
        <translation>Этот параметр отключён. Вы можете включить его снова, когда связка ключей станет доступна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="348"/>
        <source>The application secure key at %1 could not be read or rewritten.</source>
        <translation>Не удалось прочитать или перезаписать безопасный ключ приложения: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="252"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="351"/>
        <source>Please check your storage and permissions.</source>
        <translation>Проверьте хранилище и права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="200"/>
        <source>Secure Key Unavailable</source>
        <translation>Безопасный ключ недоступен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="201"/>
        <source>The application key is protected by a secret kept in the system keychain, and that secret could not be read.</source>
        <translation>Ключ приложения защищён секретом, хранящимся в системной связке ключей, и этот секрет не удалось прочитать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="205"/>
        <source>This usually means the keychain is locked, was reset, or this profile was copied from another computer or user account.</source>
        <translation>Обычно это означает, что связка ключей заблокирована, была сброшена или этот профиль был скопирован с другого компьютера или учётной записи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="209"/>
        <source>You can unlock the keychain and start the application again. Resetting the key instead lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Вы можете разблокировать связку ключей и запустить приложение снова. Сброс ключа вместо этого позволит приложению запуститься, но всё, что оно ранее зашифровало, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <source>Keychain backend: %1</source>
        <translation type="vanished">Бэкенд связки ключей: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="174"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="214"/>
        <source>Quit</source>
        <translation>Выйти</translation>
    </message>
    <message>
        <source>Everything the application has encrypted with the old key will be permanently unreadable.</source>
        <translation type="vanished">Всё, что приложение зашифровало старым ключом, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="85"/>
        <source>Reset the secure key?</source>
        <translation>Сбросить безопасный ключ?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="58"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="74"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="82"/>
        <source>Reset to Default</source>
        <translation>Сбросить к значениям по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="60"/>
        <source>Resetting removes the PIN and lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Сброс удаляет PIN и позволяет приложению запуститься, но всё, что оно ранее зашифровало, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="63"/>
        <source>Resetting lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Сброс позволяет запустить приложение, но всё, что было ранее зашифровано, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="68"/>
        <source>Only do this if you cannot recall the PIN. There is no other way to recover the key.</source>
        <translation>Делайте это только в том случае, если вы не можете вспомнить PIN. Другого способа восстановить ключ нет.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="70"/>
        <source>You can unlock the keychain and start the application again instead. That costs nothing and leaves the key intact.</source>
        <translation>Вы можете разблокировать связку ключей и запустить приложение снова. Это ничего не стоит и сохраняет ключ в целости.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="73"/>
        <source>Go Back</source>
        <translation>Назад</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="83"/>
        <source>Everything the application has encrypted with the current key will be permanently unreadable.</source>
        <translation>Всё, что приложение зашифровало текущим ключом, станет навсегда недоступно для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="96"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="103"/>
        <source>Open Profile</source>
        <translation>Открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="97"/>
        <source>That passphrase did not open this file.</source>
        <translation>Эта парольная фраза не открыла данный файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="104"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="577"/>
        <source>Enter the passphrase that protects this file:</source>
        <translation>Введите парольную фразу, защищающую этот файл:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="162"/>
        <source>It is open in process %1 on %2.</source>
        <translation>Он открыт в процессе %1 на %2.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="164"/>
        <source>this computer</source>
        <translation>этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="166"/>
        <source>Another process has it open.</source>
        <translation>Другой процесс уже открыл его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="169"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="241"/>
        <source>Profile Is Already Open</source>
        <translation>Профиль уже открыт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="170"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="242"/>
        <source>This profile is already open in another window.</source>
        <translation>Этот профиль уже открыт в другом окне.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="172"/>
        <source>Opening it twice would corrupt its stored data.</source>
        <translation>Открытие его дважды приведёт к повреждению сохранённых данных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="173"/>
        <source>Profile: %1</source>
        <translation>Профиль: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="176"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="186"/>
        <source>Force Unlock</source>
        <translation>Принудительная разблокировка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="187"/>
        <source>Only do this if you are certain no other GpgFrontend window has this profile open.</source>
        <translation>Делайте это только в том случае, если вы уверены, что ни одно другое окно GpgFrontend не открыло этот профиль.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="190"/>
        <source>If one does, both copies will corrupt the profile&apos;s stored data.</source>
        <translation>Если это произойдёт, обе копии повредят сохранённые данные профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="249"/>
        <source>Cannot Lock Profile</source>
        <translation>Не удаётся заблокировать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="250"/>
        <source>The lock file at %1 could not be created.</source>
        <translation>Не удалось создать файл блокировки %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="264"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="124"/>
        <source>This File Has Been Altered</source>
        <translation>Этот файл был изменён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="271"/>
        <source>a newer version</source>
        <translation>более новая версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="275"/>
        <source>Profile Is Too New</source>
        <translation>Профиль слишком новый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="276"/>
        <source>Profile Cannot Be Opened</source>
        <translation>Невозможно открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="277"/>
        <source>This application data was last used by %1, which stores it in a format this version does not understand.</source>
        <translation>Эти данные приложения последний раз использовались %1, который сохраняет их в формате, который эта версия не понимает.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="281"/>
        <source>Continuing would damage it. Please use %1 or later, or start this version with a different profile.</source>
        <translation>Продолжение может повредить профиль. Используйте %1 или новее, или запустите эту версию с другим профилем.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="291"/>
        <source>Profile Upgrade Failed</source>
        <translation>Не удалось обновить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="292"/>
        <source>Upgrading this profile stopped at step &apos;%1&apos;.</source>
        <translation>Обновление этого профиля остановилось на шаге &apos;%1&apos;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="295"/>
        <source>The profile is intact. Please report this.</source>
        <translation>Профиль не повреждён. Пожалуйста, сообщите об этом.</translation>
    </message>
    <message>
        <source>Continuing would damage it. Please use %1 or later, or start this version with a different data folder.</source>
        <translation type="vanished">Продолжение может повредить данные. Используйте %1 или новее, или запустите эту версию с другой папкой данных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="284"/>
        <source>Data folder: %1</source>
        <translation>Папка данных: %1</translation>
    </message>
    <message>
        <source>The application secure key at %1 could not be read.</source>
        <translation type="vanished">Не удалось прочитать ключ безопасности приложения по пути %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="392"/>
        <source>Application Key Not Protected</source>
        <translation>Ключ приложения не защищён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="393"/>
        <source>A PIN is configured, but the application key on disk is not encrypted.</source>
        <translation>Установлен PIN, но ключ приложения на диске не зашифрован.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="396"/>
        <source>This can happen if a previous change was interrupted. The PIN setting has been turned off and the key is left as it is.</source>
        <translation>Это может произойти, если предыдущее изменение было прервано. Настройка PIN отключена, и ключ оставлен как есть.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="128"/>
        <source>That PIN did not unlock the application key. Please try again.</source>
        <translation>Этот PIN-код не разблокировал ключ приложения. Пожалуйста, попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="131"/>
        <source>If you have forgotten your PIN, the application key and everything encrypted with it cannot be recovered.</source>
        <translation>Если вы забыли PIN-код, ключ приложения и всё, что с его помощью зашифровано, восстановить невозможно.</translation>
    </message>
    <message>
        <source>Failed to decrypt the application secure key. Your PIN may be incorrect, or the key file may be corrupted.Please clear the secure key and try again.</source>
        <translation type="vanished">Не удалось расшифровать ключ безопасности приложения. Возможно, вы ввели неверный PIN-код или файл ключа повреждён. Пожалуйста, удалите ключ безопасности и повторите попытку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="55"/>
        <source>Active</source>
        <translation>Активен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="56"/>
        <source>Inactive</source>
        <translation>Неактивен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="65"/>
        <source>%1  ·  %2% used  ·  %3</source>
        <translation>%1  ·  Использовано %2%  ·  %3</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="71"/>
        <source>%1  ·  %2</source>
        <translation>%1  ·  %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="52"/>
        <source>%1 item(s)</source>
        <translation>%1 элемент(ов)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="148"/>
        <source>Open Link</source>
        <translation>Открыть ссылку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="149"/>
        <source>Could not open a web browser on this system.

The link has been copied to your clipboard:
%1</source>
        <translation>Не удалось открыть веб-браузер в этой системе.

Ссылка скопирована в буфер обмена:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="177"/>
        <source>Star GpgFrontend on GitHub</source>
        <translation>Поставить звезду GpgFrontend на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="184"/>
        <source>GpgFrontend is free and open source. A star helps more people discover it and keeps the project moving forward.</source>
        <translation>GpgFrontend — бесплатное приложение с открытым исходным кодом. Звезда помогает большему числу пользователей найти его и способствует развитию проекта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="76"/>
        <source>Very weak</source>
        <translation>Очень слабый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="79"/>
        <source>Weak</source>
        <translation>Слабый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="82"/>
        <source>Fair</source>
        <translation>Средний</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="85"/>
        <source>Good</source>
        <translation>Хороший</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/PassphraseStrength.cpp" line="87"/>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="755"/>
        <source>Strong</source>
        <translation>Сильный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="99"/>
        <source>Fully Valid</source>
        <translation>Полностью действительный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="101"/>
        <source>Valid (with Issues)</source>
        <translation>Действительный (с замечаниями)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="103"/>
        <source>Valid (Not Fully Trusted)</source>
        <translation>Действительный (неполное доверие)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="105"/>
        <source>Invalid</source>
        <translation>Недействительный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="107"/>
        <source>Key Missing</source>
        <translation>Ключ отсутствует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="109"/>
        <source>Key Revoked</source>
        <translation>Ключ отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="111"/>
        <source>Signature Expired</source>
        <translation>Срок действия подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="113"/>
        <source>Key Expired</source>
        <translation>Срок действия ключа истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="115"/>
        <source>Verification Error</source>
        <translation>Ошибка проверки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="117"/>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="759"/>
        <source>Unknown</source>
        <translation>Неизвестно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="123"/>
        <source>Signer</source>
        <translation>Подписант</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="125"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="146"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="193"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="127"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="148"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="133"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="151"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="136"/>
        <source>Signed</source>
        <translation>Подписано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="144"/>
        <source>Recipient</source>
        <translation>Получатель</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="150"/>
        <source>Primary Key Algorithm</source>
        <translation>Алгоритм первичного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="163"/>
        <source>Input Material Hash</source>
        <translation>Хеш входных данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="165"/>
        <source>SHA-256</source>
        <translation>SHA-256</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="182"/>
        <source>Signature Created</source>
        <translation>Подпись создана</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="185"/>
        <source>Mode</source>
        <translation>Режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="191"/>
        <source>Invalid Signer</source>
        <translation>Недействительный подписант</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="194"/>
        <source>Reason</source>
        <translation>Причина</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="201"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="229"/>
        <source>Decrypt</source>
        <translation>Расшифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="204"/>
        <source>Message Metadata</source>
        <translation>Метаданные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="207"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="208"/>
        <source>Cipher</source>
        <translation>Шифр</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="209"/>
        <source>MIME</source>
        <translation>MIME</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="210"/>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="211"/>
        <source>No</source>
        <translation>Нет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="212"/>
        <source>Integrity</source>
        <translation>Целостность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="214"/>
        <source>Protected</source>
        <translation>Защищён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="215"/>
        <source>Not Protected (unsafe)</source>
        <translation>Не защищён (небезопасно)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="219"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="222"/>
        <source>Encryption Recipient</source>
        <translation>Получатель шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="232"/>
        <source>Decryption Recipient</source>
        <translation>Получатель расшифровки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="196"/>
        <source>ECC</source>
        <translation>ECC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="203"/>
        <source>Post-Quantum</source>
        <translation>Постквантовый</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProfileSecureKeyManager.cpp" line="460"/>
        <source>The secure key could not be encrypted, so it was not saved to disk.</source>
        <translation>Защищённый ключ не удалось зашифровать, поэтому он не был сохранён на диск.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="751"/>
        <source>Standard</source>
        <translation>Стандартный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="753"/>
        <source>Enhanced</source>
        <translation>Расширенный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="757"/>
        <source>Maximum</source>
        <translation>Максимальный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="766"/>
        <source>System keychain</source>
        <translation>Системная связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="768"/>
        <source>PIN at startup</source>
        <translation>PIN при запуске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/UserInterfaceUtils.cpp" line="772"/>
        <source>No extra protection</source>
        <translation>Без дополнительной защиты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="98"/>
        <source>Home</source>
        <translation>Главная</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="86"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="91"/>
        <source>The new window could not be started.</source>
        <translation>Не удалось открыть новое окно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="100"/>
        <source>Name This Profile</source>
        <translation>Назвать этот профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="101"/>
        <source>What should this profile be called on this computer?</source>
        <translation>Как назвать этот профиль на этом компьютере?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="125"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="145"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="167"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="567"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="590"/>
        <source>Cannot Import Profile</source>
        <translation>Не удалось импортировать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="146"/>
        <source>This profile was made by a newer version of GpgFrontend (%1).</source>
        <translation>Этот профиль создан более новой версией GpgFrontend (%1).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="174"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="638"/>
        <source>&quot;%1&quot; is ready.</source>
        <translation>&quot;%1&quot; готов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="177"/>
        <source>The file did not carry any workspace files.</source>
        <translation>Файл не содержал файлов рабочей области.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="182"/>
        <source>&quot;%1&quot; pointed at keys kept outside the profile, which do not travel. It will show as unavailable until you point it somewhere on this computer.</source>
        <translation>&quot;%1&quot; указывал на ключи, хранящиеся вне профиля, которые не переносятся. Он будет отображаться как недоступный, пока вы не укажете на него где-то на этом компьютере.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="191"/>
        <source>Profile Imported</source>
        <translation>Профиль импортирован</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="193"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="639"/>
        <source>Open it now? It opens in a new window.</source>
        <translation>Открыть его сейчас? Он откроется в новом окне.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="237"/>
        <source>GpgFrontend Profile File</source>
        <translation>Файл профиля GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="306"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="321"/>
        <source>Default</source>
        <translation>По умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="308"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="323"/>
        <source>Portable</source>
        <translation>Портативный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="310"/>
        <source>From a package</source>
        <translation>Из пакета</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="312"/>
        <source>Local</source>
        <translation>Локальный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="359"/>
        <source>This file is no longer there:</source>
        <translation>Этого файла больше нет:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="368"/>
        <source>This window is already using that profile.</source>
        <translation>Это окно уже использует этот профиль.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="378"/>
        <source>There is no profile called &quot;%1&quot;.</source>
        <translation>Нет профиля с именем «%1».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="397"/>
        <source>&quot;%1&quot; is open in another window (process %2 on %3).</source>
        <translation>«%1» открыт в другом окне (процесс %2 на %3).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="402"/>
        <source>&quot;%1&quot; is open in another window.</source>
        <translation>«%1» открыт в другом окне.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="434"/>
        <source>Save Changes?</source>
        <translation>Сохранить изменения?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="435"/>
        <source>This profile was opened from a file. It is not kept on this computer, and the copy it is running from is about to be deleted.</source>
        <translation>Этот профиль был открыт из файла. Он не хранится на этом компьютере, и копия, из которой он запущен, будет удалена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="439"/>
        <source>Anything you changed is lost unless it is written back into that file.</source>
        <translation>Любые изменения будут потеряны, если их не записать обратно в этот файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="443"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="466"/>
        <source>Save Changes</source>
        <translation>Сохранить изменения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="445"/>
        <source>Discard</source>
        <translation>Отбросить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="446"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="467"/>
        <source>Enter the passphrase to protect this file with:</source>
        <translation>Введите парольную фразу для защиты этого файла:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="518"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="540"/>
        <source>Cannot Save Changes</source>
        <translation>Не удалось сохранить изменения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="519"/>
        <source>The application key is not available, so the profile could not be packed.</source>
        <translation>Ключ приложения недоступен, поэтому профиль не удалось упаковать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="527"/>
        <source>Saving Profile</source>
        <translation>Сохранение профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="558"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="576"/>
        <source>Import Profile File</source>
        <translation>Импорт файла профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="591"/>
        <source>A temporary folder could not be made.</source>
        <translation>Не удалось создать временную папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="598"/>
        <source>Reading Profile</source>
        <translation>Чтение профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="627"/>
        <source>Cannot Create Profile</source>
        <translation>Не удалось создать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="628"/>
        <source>The profile could not be created.</source>
        <translation>Профиль не удалось создать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="637"/>
        <source>Profile Created</source>
        <translation>Профиль создан</translation>
    </message>
</context>
<context>
    <name>QWizardPage</name>
    <message>
        <source>A simple, privacy-focused OpenPGP tool for text, files, and keys.</source>
        <translation type="vanished">Простой инструмент OpenPGP, ориентированный на конфиденциальность, для текста, файлов и ключей.</translation>
    </message>
</context>
</TS>
