<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
<context>
    <name>GpgFrontend</name>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1510"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1512"/>
        <source>Expiring Soon</source>
        <translation>Скоро истекает</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1514"/>
        <source>Expired</source>
        <translation>Срок истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1516"/>
        <source>Revoked</source>
        <translation>Отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1518"/>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgDecryptResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgOpResultInfo.cpp" line="57"/>
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
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="80"/>
        <source>Cipher Warning</source>
        <translation>Предупреждение о шифре</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="88"/>
        <source>General State</source>
        <translation>Общее состояние</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="95"/>
        <source>File Name</source>
        <translation>Имя файла</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="98"/>
        <source>MIME</source>
        <translation>MIME</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="99"/>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="102"/>
        <source>false</source>
        <translation>нет</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="99"/>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="102"/>
        <source>true</source>
        <translation>да</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="101"/>
        <source>Message Integrity Protection</source>
        <translation>Защита целостности сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="106"/>
        <source>Symmetric Encryption Algorithm</source>
        <translation>Алгоритм симметричного шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="113"/>
        <source>Recipient(s)</source>
        <translation>Получатель(и)</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="120"/>
        <source>Recipient</source>
        <translation>Получатель</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="135"/>
        <source>Decrypted successfully. The original filename is &quot;%1&quot;.</source>
        <translation>Расшифровка выполнена успешно. Исходное имя файла: &quot;%1&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="138"/>
        <source>The message has been decrypted successfully and is now readable.</source>
        <translation>Сообщение успешно расшифровано и теперь доступно для чтения.</translation>
    </message>
    <message>
        <source>Decrypted, but message integrity protection is absent. This may be a legacy message that cannot detect tampering.</source>
        <translation type="vanished">Расшифровано, но защита целостности сообщения отсутствует. Возможно, это устаревшее сообщение, в котором невозможно обнаружить подмену.</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="148"/>
        <source>Decrypted, but %n recipient key(s) could not be found in your keyring.</source>
        <translation>
            <numerusform>Расшифровано, но в вашей связке ключей не найдено %n ключ(ей) получателя(ей).</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="151"/>
        <source>Decrypted with warnings. Please review the details.</source>
        <translation>Расшифровано с предупреждениями. Пожалуйста, ознакомьтесь с деталями.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="154"/>
        <source>The output was withheld: this message has no integrity protection, so tampering with it cannot be detected. It may be a legacy message.</source>
        <translation>Результат не выдан: у этого сообщения нет защиты целостности, поэтому его подделку невозможно обнаружить. Возможно, это устаревшее сообщение.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="160"/>
        <source>Decryption failed: %1.</source>
        <translation>Расшифровка не удалась: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="187"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;unknown&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="195"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="199"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="200"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="206"/>
        <source>Public Key Algo</source>
        <translation>Алгоритм открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgDecryptResultAnalyse.cpp" line="208"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgEncryptResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgOpResultInfo.cpp" line="53"/>
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
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="69"/>
        <source>Invalid Recipients</source>
        <translation>Недопустимые получатели</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="74"/>
        <source>Recipients</source>
        <translation>Получатели</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="76"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="77"/>
        <source>Reason</source>
        <translation>Причина</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="90"/>
        <source>Encrypted with warnings. Some recipients could not be added. Please review the invalid recipient list.</source>
        <translation>Зашифровано с предупреждениями. Некоторые получатели не были добавлены. Пожалуйста, проверьте список недопустимых получателей.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="92"/>
        <source>Your data has been encrypted. Only the intended recipients can decrypt and read it.</source>
        <translation>Ваши данные зашифрованы. Только предназначенные получатели могут расшифровать и прочитать их.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgEncryptResultAnalyse.cpp" line="97"/>
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
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="247"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="44"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="248"/>
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
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="249"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="47"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="108"/>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="168"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="245"/>
        <source>ID</source>
        <translation>ID</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="246"/>
        <source>Algo</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="250"/>
        <source>No Comment</source>
        <translation>Нет комментария</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="256"/>
        <source>SubKeys (up to 8):</source>
        <translation>Подключи (до 8):</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTableModel.cpp" line="262"/>
        <source>ID: %1 | Algo: %2 | Usage: %3</source>
        <translation>ID: %1 | Алгоритм: %2 | Использование: %3</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgKeyTreeModel</name>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="56"/>
        <source>Select</source>
        <translation>Выбрать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="57"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="58"/>
        <source>Identity</source>
        <translation>Идентичность</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="59"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="60"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="61"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="62"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKeyTreeModel.cpp" line="336"/>
        <source>(already shown above)</source>
        <translation>Дата истечения</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::GpgSignResultAnalyse</name>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgOpResultInfo.cpp" line="61"/>
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
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="429"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="469"/>
        <source>UTC</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="68"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="432"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="472"/>
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
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="135"/>
        <source>Tips</source>
        <translation>Подсказки</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="239"/>
        <source>Could not find information that can be used for verification.</source>
        <translation>Не удалось найти информацию, которую можно использовать для проверки.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="421"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="447"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="460"/>
        <source>Signed By</source>
        <translation>Подписано</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="421"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="460"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="423"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="461"/>
        <source>&lt;unknown&gt;</source>
        <translation>&lt;неизвестно&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="426"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="466"/>
        <source>Public Key Algo</source>
        <translation>Алгоритм открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="428"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="468"/>
        <source>Hash Algo</source>
        <translation>Алгоритм хеширования</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="429"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="432"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="469"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="472"/>
        <source>Sign Date</source>
        <translation>Дата подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="143"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="450"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="453"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgOpResultInfo.cpp" line="65"/>
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
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="100"/>
        <source>Valid</source>
        <translation>Действительна</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="102"/>
        <source>Fully Valid</source>
        <translation>Полностью действителен</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="104"/>
        <source>Valid but with Issues</source>
        <translation>Действительна, но с проблемами</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="106"/>
        <source>Valid but Not Fully Trusted</source>
        <translation>Действительна, но не полностью доверена</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="112"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="291"/>
        <source>Signature has expired</source>
        <translation>Срок действия подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="115"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="293"/>
        <source>Signing key is missing</source>
        <translation>Ключ подписи отсутствует</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="118"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="295"/>
        <source>Signing key has been revoked</source>
        <translation>Ключ подписи был отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="121"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="297"/>
        <source>Signing key has expired</source>
        <translation>Срок действия ключа подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="124"/>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="299"/>
        <source>Certificate revocation list is missing</source>
        <translation>Отсутствует список отзыва сертификатов</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="128"/>
        <source>Warnings</source>
        <translation>Предупреждения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="136"/>
        <source>Adjust trust level to make it fully valid</source>
        <translation>Настройте уровень доверия для полной валидности</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="144"/>
        <source>not present in keyring</source>
        <translation>отсутствует в связке ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="151"/>
        <source>Cannot Verify due to Key Missing</source>
        <translation>Невозможно проверить из-за отсутствия ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="153"/>
        <source>The public key needed to verify this signature is not available</source>
        <translation>Открытый ключ, необходимый для проверки этой подписи, недоступен</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="162"/>
        <source>Key Revoked</source>
        <translation>Ключ отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="164"/>
        <source>The signature is mathematically valid but the signing key has been revoked</source>
        <translation>Подпись математически верна, но ключ подписи был отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="173"/>
        <source>Signature Expired</source>
        <translation>Срок действия подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="174"/>
        <source>The signature is valid but has expired</source>
        <translation>Подпись верна, но срок её действия истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="182"/>
        <source>Signing Key Expired</source>
        <translation>Срок действия ключа подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="187"/>
        <source>The signing key had already expired when this signature was made</source>
        <translation>Срок действия ключа подписи истёк ещё до создания этой подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="193"/>
        <source>The signature is valid but the signing key has expired</source>
        <translation>Подпись верна, но срок действия ключа подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="202"/>
        <source>Signing Key Not Authorized</source>
        <translation>Ключ подписи не уполномочен</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="204"/>
        <source>The signature is mathematically valid but the certificate does not authorize this key to sign</source>
        <translation>Подпись математически верна, но сертификат не разрешает этому ключу подписывать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="213"/>
        <source>Verification Error</source>
        <translation>Ошибка проверки</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="215"/>
        <source>An error occurred during signature verification</source>
        <translation>Произошла ошибка при проверке подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="222"/>
        <source>Unknown Error</source>
        <translation>Неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="223"/>
        <source>Key Fingerprint</source>
        <translation>Отпечаток ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="230"/>
        <source>Verification process stopped due to errors.</source>
        <translation>Процесс проверки остановлен из-за ошибок.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="243"/>
        <source>No verifiable signatures were found in the data.</source>
        <translation>В данных не обнаружено подписей для проверки.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="317"/>
        <source>Signing key is not authorized by its certificate</source>
        <translation>Сертификат не уполномочивает ключ подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="355"/>
        <source>The signature is valid. The data has not been tampered with.</source>
        <translation>Подпись действительна. Данные не были изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="357"/>
        <source>The signature by %1 is valid. The data has not been tampered with.</source>
        <translation>Подпись %1 действительна. Данные не были изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="365"/>
        <source>The signature is fully valid and trusted. The data has not been tampered with.</source>
        <translation>Подпись полностью действительна и доверена. Данные не были изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="367"/>
        <source>The signature by %1 is fully valid and trusted. The data has not been tampered with.</source>
        <translation>Подпись от %1 полностью действительна и доверена. Данные не были изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="372"/>
        <source>The signature is valid, but the signing key is not fully trusted. You may need to set a higher trust level.</source>
        <translation>Подпись действительна, но ключ подписи не полностью доверен. Возможно, потребуется повысить уровень доверия.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="378"/>
        <source>Verification incomplete. The signing key is not in your keyring. Please import the signer&apos;s public key.</source>
        <translation>Проверка не завершена. Ключа подписи нет в вашей связке ключей. Импортируйте открытый ключ подписанта.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="382"/>
        <source>The signature is valid but the signing key is not fully trusted. Consider setting a higher trust level for that key.</source>
        <translation>Подпись действительна, но ключ подписи не полностью доверен. Рекомендуется повысить уровень доверия для этого ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="386"/>
        <source>Verification completed with warnings. Please review the details.</source>
        <translation>Проверка завершена с предупреждениями. Ознакомьтесь с подробностями.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="392"/>
        <source>Invalid signature detected. The data may have been tampered with or the signature is corrupt.</source>
        <translation>Обнаружена недействительная подпись. Данные могли быть изменены или подпись повреждена.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="396"/>
        <source>Signature verification failed. The signing key is not authorized to make signatures.</source>
        <translation>Проверка подписи не удалась. Ключ подписи не уполномочен создавать подписи.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="400"/>
        <source>Signature verification failed. The signing key has been revoked.</source>
        <translation>Проверка подписи не удалась. Ключ подписи отозван.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="404"/>
        <source>Signature verification failed. This signature has expired.</source>
        <translation>Проверка подписи не удалась. Срок действия подписи истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="407"/>
        <source>Signature verification failed. The signing key has expired.</source>
        <translation>Проверка подписи не удалась. Срок действия ключа подписи истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="410"/>
        <source>Cannot verify. The signing key is not available in your keyring.</source>
        <translation>Невозможно проверить. Ключ подписи отсутствует в вашей связке ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="414"/>
        <source>Signature verification failed. Please review the details.</source>
        <translation>Проверка подписи не удалась. Ознакомьтесь с подробностями.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="451"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="454"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/result_analyse/GpgVerifyResultAnalyse.cpp" line="456"/>
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
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="308"/>
        <source>Leaf</source>
        <translation>Лист</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="312"/>
        <source>Namespace</source>
        <translation>Пространство имён</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="344"/>
        <source>Empty</source>
        <translation>Пусто</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="349"/>
        <source>String</source>
        <translation>Строка</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="353"/>
        <source>Boolean</source>
        <translation>Логический</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="359"/>
        <source>Integer</source>
        <translation>Целое число</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="363"/>
        <source>Number</source>
        <translation>Число</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="366"/>
        <source>Unsupported</source>
        <translation>Не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="411"/>
        <source>&lt;UNSUPPORTED&gt;</source>
        <translation>&lt;НЕ ПОДДЕРЖИВАЕТСЯ&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="448"/>
        <source>Key</source>
        <translation>Ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="451"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="455"/>
        <source>Value Type</source>
        <translation>Тип значения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="459"/>
        <source>Value</source>
        <translation>Значение</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/GlobalRegisterTable.cpp" line="463"/>
        <source>Version</source>
        <translation>Версия</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::SettingItem</name>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="74"/>
        <source>Default OpenPGP engine</source>
        <translation>Движок OpenPGP по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="83"/>
        <source>Use a custom GnuPG installation</source>
        <translation>Использовать пользовательскую установку GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="93"/>
        <source>Custom GnuPG installation</source>
        <translation>Пользовательская установка GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="103"/>
        <source>GPGME debug log</source>
        <translation>Отладочный журнал GPGME</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="111"/>
        <source>Stop GnuPG daemons on exit</source>
        <translation>Останавливать демоны GnuPG при выходе</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="119"/>
        <source>Binary output for file operations</source>
        <translation>Двоичный вывод для файловых операций</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="128"/>
        <source>Keep GnuPG offline</source>
        <translation>Держать GnuPG в автономном режиме</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="135"/>
        <source>Forget cached passphrases on exit</source>
        <translation>Забывать кэшированные парольные фразы при выходе</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="146"/>
        <source>Passphrase cache time</source>
        <translation>Время кэширования парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="153"/>
        <source>Longest passphrase cache time</source>
        <translation>Максимальное время кэширования парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="161"/>
        <source>Passphrase prompt timeout</source>
        <translation>Тайм-аут запроса парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="169"/>
        <source>Passphrase requester grace period</source>
        <translation>Льготный период запроса парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="177"/>
        <source>Argon2 key derivation strength</source>
        <translation>Стойкость формирования ключа Argon2</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="187"/>
        <source>Confirm before importing keys</source>
        <translation>Подтверждать перед импортом ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="196"/>
        <source>Single recipient mode</source>
        <translation>Режим одного получателя</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="203"/>
        <source>Confirm encrypting to several recipients</source>
        <translation>Подтверждать шифрование нескольким получателям</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="212"/>
        <source>Remember checked keys</source>
        <translation>Запоминать отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="220"/>
        <source>Undoable operations</source>
        <translation>Отменяемые операции</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="227"/>
        <source>Restore text editor pages</source>
        <translation>Восстанавливать страницы текстового редактора</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="235"/>
        <source>Default workspace view</source>
        <translation>Вид рабочей области по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="243"/>
        <source>File panel starting folder</source>
        <translation>Начальная папка панели файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="252"/>
        <source>Show the file type column</source>
        <translation>Показывать столбец типа файла</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="260"/>
        <source>Days before a key counts as expiring soon</source>
        <translation>Дней до того, как ключ считается скоро истекающим</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="268"/>
        <source>Fetch missing keys automatically</source>
        <translation>Автоматически получать отсутствующие ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="277"/>
        <source>Which modules to load</source>
        <translation>Какие модули загружать</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="287"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="295"/>
        <source>Theme</source>
        <translation>Тема</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="303"/>
        <source>Show the first-run wizard</source>
        <translation>Показывать мастер первого запуска</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="313"/>
        <source>Log level</source>
        <translation>Уровень журналирования</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="322"/>
        <source>Log lines kept in memory</source>
        <translation>Строк журнала в памяти</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="333"/>
        <source>Secure level</source>
        <translation>Уровень безопасности</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="341"/>
        <source>Application key protection</source>
        <translation>Защита ключа приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="353"/>
        <source>System credential store (legacy)</source>
        <translation>Системное хранилище учётных данных (устаревшее)</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="363"/>
        <source>Where profile files are opened</source>
        <translation>Где открываются файлы профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="373"/>
        <source>Never check for updates</source>
        <translation>Никогда не проверять обновления</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="380"/>
        <source>Use a proxy</source>
        <translation>Использовать прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="386"/>
        <source>Proxy type</source>
        <translation>Тип прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="392"/>
        <source>Proxy host</source>
        <translation>Хост прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="398"/>
        <source>Proxy port</source>
        <translation>Порт прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="404"/>
        <source>Proxy user name</source>
        <translation>Имя пользователя прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="411"/>
        <source>Proxy password</source>
        <translation>Пароль прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="420"/>
        <source>Workspace folder</source>
        <translation>Папка рабочей области</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="539"/>
        <source>OpenPGP engine and GnuPG</source>
        <translation>Механизм OpenPGP и GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="542"/>
        <source>rPGP passphrase handling</source>
        <translation>Обработка парольной фразы rPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="545"/>
        <source>Everyday behaviour</source>
        <translation>Повседневная работа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="548"/>
        <source>Language and appearance</source>
        <translation>Язык и оформление</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="551"/>
        <source>Diagnostics</source>
        <translation>Диагностика</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="554"/>
        <source>Key protection</source>
        <translation>Защита ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="557"/>
        <source>Network and proxy</source>
        <translation>Сеть и прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="560"/>
        <source>Workspace</source>
        <translation>Рабочая область</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="563"/>
        <source>Remembered layout</source>
        <translation>Запоминаемая раскладка</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="574"/>
        <source>Which engine runs, which GnuPG it uses, and how GnuPG behaves.</source>
        <translation>Какой движок работает, какой GnuPG он использует и как ведёт себя GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="578"/>
        <source>How long passphrases are kept and how keys are derived.</source>
        <translation>Как долго хранятся парольные фразы и как выводятся ключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="582"/>
        <source>Confirmations, recipients, the editor and the file panel.</source>
        <translation>Подтверждения, получатели, редактор и панель файлов.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="586"/>
        <source>Language, theme and the first-run wizard.</source>
        <translation>Язык, тема и мастер первого запуска.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="590"/>
        <source>How much is written to the log.</source>
        <translation>Сколько записывается в журнал.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="593"/>
        <source>How this profile&apos;s own key is protected.</source>
        <translation>Как защищён собственный ключ этого профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="597"/>
        <source>Update checks and the proxy.</source>
        <translation>Проверка обновлений и прокси.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="600"/>
        <source>Where files are kept.</source>
        <translation>Где хранятся файлы.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/function/SettingCatalog.h" line="603"/>
        <source>Remembered tab orders and column choices.</source>
        <translation>Запоминаемые порядок вкладок и набор столбцов.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI</name>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="237"/>
        <source>Short Name</source>
        <translation>Короткое имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="238"/>
        <source>The name &quot;%1&quot; is shorter than five characters. Short names are allowed, but they are often a typo and make the key harder for others to recognise.

Do you want to continue?</source>
        <translation>Имя &quot;%1&quot; короче пяти символов. Короткие имена допускаются, но часто являются опечаткой и затрудняют распознавание ключа другим пользователям.

Вы хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="50"/>
        <source>Signature (OPENPGP.1)</source>
        <translation>Подпись (OPENPGP.1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="53"/>
        <source>Encryption (OPENPGP.2)</source>
        <translation>Шифрование (OPENPGP.2)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="56"/>
        <source>Authentication (OPENPGP.3)</source>
        <translation>Аутентификация (OPENPGP.3)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="75"/>
        <source>Select Card Slot</source>
        <translation>Выберите слот карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="76"/>
        <source>This key can be stored in more than one slot. Where should it be stored?</source>
        <translation>Этот ключ можно сохранить более чем в одном слоте. Где его следует сохранить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="97"/>
        <source>No Smart Card</source>
        <translation>Нет смарт-карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="98"/>
        <source>No OpenPGP smart card was detected. Insert a card and try again.</source>
        <translation>Смарт-карта OpenPGP не обнаружена. Вставьте карту и повторите попытку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="108"/>
        <source>Select Smart Card</source>
        <translation>Выберите смарт-карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="109"/>
        <source>Move the key to which card?</source>
        <translation>На какую карту переместить ключ?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="124"/>
        <source>Back Up Secret Key</source>
        <translation>Создать резервную копию секретного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="125"/>
        <source>Do you want to export a backup of the secret key before moving it to the card? After the move the key can only be used through the card.</source>
        <translation>Вы хотите экспортировать резервную копию секретного ключа перед перемещением на карту? После перемещения ключ можно будет использовать только через карту.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="130"/>
        <source>Back Up First</source>
        <translation>Сначала создать резервную копию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="133"/>
        <source>Continue Without Backup</source>
        <translation>Продолжить без резервной копии</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="155"/>
        <source>Export Secret Key Backup</source>
        <translation>Экспорт резервной копии секретного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="158"/>
        <source>Key Files</source>
        <translation>Файлы ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="164"/>
        <source>Export Error</source>
        <translation>Ошибка экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="165"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="187"/>
        <source>Move Key to Smart Card</source>
        <translation>Переместить ключ на смарт-карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="191"/>
        <source>&lt;h3&gt;You are about to move a private key onto a smart card.&lt;/h3&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br/&gt;&lt;br/&gt;This &lt;b&gt;moves&lt;/b&gt; the key: its private part is removed from this computer and only a card reference (stub) remains. Afterwards the key can only be used through the card. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br/&gt;&lt;br/&gt;Do you want to continue?</source>
        <translation>&lt;h3&gt;Вы собираетесь переместить закрытый ключ на смарт-карту.&lt;/h3&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br/&gt;&lt;br/&gt;Это &lt;b&gt;перемещает&lt;/b&gt; ключ: его закрытая часть удаляется с этого компьютера, и остаётся только ссылка на карту (заглушка). После этого ключ можно будет использовать только через карту. Это действие &lt;b&gt;необратимо&lt;/b&gt;.&lt;br/&gt;&lt;br/&gt;Вы хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="214"/>
        <source>This key has no capability that can be stored on a smart card.</source>
        <translation>У этого ключа нет возможностей, которые можно сохранить на смарт-карте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/MoveKeyToCard.cpp" line="242"/>
        <source>The key was moved to the smart card successfully.</source>
        <translation>Ключ был успешно перемещён на смарт-карту.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="93"/>
        <source>No Supported OpenPGP Engine Found</source>
        <translation>Поддерживаемый движок OpenPGP не найден</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="95"/>
        <source>It seems that no supported OpenPGP engine is available. Please check your if GpgFrontend is properly installed and try again. Reason: %1</source>
        <translation>Похоже, что поддерживаемый движок OpenPGP недоступен. Пожалуйста, проверьте правильность установки GpgFrontend и попробуйте снова. Причина: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="40"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <source>Operation Not Supported</source>
        <translation type="vanished">Операция не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="53"/>
        <source>Failure</source>
        <translation>Неудача</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="67"/>
        <source>Open Key</source>
        <translation>Открыть ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="69"/>
        <source>Keyring files</source>
        <translation>Файлы связки ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="77"/>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="87"/>
        <location filename="../../../../src/ui/function/ShowKeyDetails.cpp" line="41"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="78"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Невозможно открыть этот файл. Убедитесь, что это обычный файл и он доступен для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="88"/>
        <source>The target file is too large for a keyring.</source>
        <translation>Целевой файл слишком большой для связки ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="97"/>
        <source>File Open Failed</source>
        <translation>Не удалось открыть файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ImportKey.cpp" line="98"/>
        <source>Failed to open file: </source>
        <translation>Не удалось открыть файл: </translation>
    </message>
    <message>
        <source>Processing</source>
        <translation type="vanished">Обработка</translation>
    </message>
    <message>
        <source>Failed to execute command.</source>
        <translation type="vanished">Не удалось выполнить команду.</translation>
    </message>
    <message>
        <source>Succeed in executing command.</source>
        <translation type="vanished">Команда выполнена успешно.</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation type="vanished">Предупреждение</translation>
    </message>
    <message>
        <source>Finished executing command.</source>
        <translation type="vanished">Выполнение команды завершено.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ShowKeyDetails.cpp" line="42"/>
        <source>Key Not Found.</source>
        <translation>Ключ не найден.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="54"/>
        <source>Gpg Operation failed.</source>
        <translation>Операция GPG не удалась.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="51"/>
        <source>No Usable Key Database</source>
        <translation>Нет доступной базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="53"/>
        <source>None of the configured key databases could be opened. This usually means the folder was moved or deleted, or is on a drive that is not currently available.</source>
        <translation>Ни одну из настроенных баз данных ключей не удалось открыть. Обычно это означает, что папка была перемещена, удалена или находится на диске, который в данный момент недоступен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="59"/>
        <source>You can change where your key databases live in Settings, under Key Databases. Details: %1</source>
        <translation>Вы можете изменить расположение баз данных ключей в настройках, в разделе «Базы данных ключей». Подробности: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="68"/>
        <source>Cannot Prepare Application Data</source>
        <translation>Невозможно подготовить данные приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="70"/>
        <source>GpgFrontend could not set up the folders it needs to store its data. Please check that the application data folder is writable. Details: %1</source>
        <translation>GpgFrontend не удалось настроить папки, необходимые для хранения данных. Проверьте, что папка данных приложения доступна для записи. Подробности: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="79"/>
        <source>Key Database Could Not Be Opened</source>
        <translation>Не удалось открыть базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/OpenPGPEnvGuard.cpp" line="81"/>
        <source>The key database was found but could not be loaded. It may be in use by another program, or its permissions may have changed. Details: %1</source>
        <translation>База данных ключей найдена, но не может быть загружена. Возможно, она используется другой программой или были изменены её права доступа. Подробности: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="41"/>
        <source>Operation completed successfully.</source>
        <translation>Операция успешно завершена.</translation>
    </message>
    <message>
        <source>The current OpenPGP engine does not support this operation. Please use a supported engine or upgrade the engine version.</source>
        <translation type="vanished">Текущий движок OpenPGP не поддерживает эту операцию. Пожалуйста, используйте поддерживаемый движок или обновите версию движка.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="56"/>
        <source>Error code: %1</source>
        <translation>Код ошибки: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="59"/>
        <source>Source:  %1</source>
        <translation>Источник:  %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="62"/>
        <source>Description: %1</source>
        <translation>Описание: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgErrorMessageBox.cpp" line="65"/>
        <source>Error Message: %1</source>
        <translation>Сообщение об ошибке: %1</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ADSKsPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="65"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="66"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="77"/>
        <source>No Subkeys Selected</source>
        <translation>Подключи не выбраны</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="78"/>
        <source>Please select at least one Subkey.</source>
        <translation>Выберите хотя бы один подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="88"/>
        <source>Select ADSK(s)</source>
        <translation>Выберите ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="93"/>
        <source>ADSK (Additional Decryption Subkey) allows others to encrypt data for you without having access to your private key. You are only allow to check subkeys with encryption capability.</source>
        <translation>ADSK (дополнительный подключ расшифровки) позволяет другим шифровать данные для вас без доступа к вашему закрытому ключу. Допускается выбор только подключей с возможностью шифрования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="117"/>
        <source>ADSKs Picker</source>
        <translation>Выбор ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="137"/>
        <source>Key ID: %1 Reason: %2</source>
        <translation>Идентификатор ключа: %1 Причина: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="149"/>
        <source>Unknown Error</source>
        <translation>Неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="150"/>
        <source>Caught unknown error while exporting the key.</source>
        <translation>Обнаружена неизвестная ошибка при экспорте ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="167"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="168"/>
        <source>Partially Failed</source>
        <translation>Частично не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="171"/>
        <source>Failed to add all selected subkeys.</source>
        <translation>Не удалось добавить все выбранные подключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/ADSKsPicker.cpp" line="172"/>
        <source>Some subkeys failed to be added as ADSKs.</source>
        <translation>Не удалось добавить некоторые подключи как ADSK.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AboutDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="192"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="202"/>
        <source>About</source>
        <translation>О программе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="203"/>
        <source>Build Information</source>
        <translation>Сведения о сборке</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="205"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="706"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="208"/>
        <source>Rust Engine</source>
        <translation>Rust-движок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="204"/>
        <source>Translators</source>
        <translation>Переводчики</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AboutStatusInfo</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="41"/>
        <source>Memory</source>
        <translation>Память</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="43"/>
        <source>Not written to this disk in the normal course of things.</source>
        <translation>Не записывается на этот диск в обычном порядке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="50"/>
        <source>An encrypted folder</source>
        <translation>Зашифрованная папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="52"/>
        <source>Only this session can read it.</source>
        <translation>Только этот сеанс может его прочитать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="57"/>
        <source>An ordinary folder on this disk</source>
        <translation>Обычная папка на этом диске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="59"/>
        <source>Nothing here could hold it in memory or encrypt it.</source>
        <translation>Ничто здесь не может удержать это в памяти или зашифровать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="69"/>
        <source>Profile:</source>
        <translation>Профиль:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="71"/>
        <source>Profile Type:</source>
        <translation>Тип профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="81"/>
        <source>The profile this computer starts on, in your user data folder.</source>
        <translation>Профиль, с которого запускается этот компьютер, в вашей папке пользовательских данных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="92"/>
        <source>Kept beside the application, so it travels with it.</source>
        <translation>Хранится рядом с приложением, поэтому переносится вместе с ним.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="105"/>
        <source>Opened from a profile file</source>
        <translation>Открыт из файла профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="109"/>
        <source>Temporary. It disappears when this window closes, and closing asks whether to save your changes back into the file.</source>
        <translation>Временный. Исчезает при закрытии этого окна, и при закрытии запрашивается сохранение изменений обратно в файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="120"/>
        <source>Temporary</source>
        <translation>Временный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="122"/>
        <source>Not kept on this computer. Everything in it is destroyed when this window closes.</source>
        <translation>Не хранится на этом компьютере. Всё содержимое уничтожается при закрытии этого окна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="131"/>
        <source>Kept on this computer</source>
        <translation>Хранится на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="141"/>
        <source>System keychain</source>
        <translation>Системная связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="145"/>
        <source>PIN at startup</source>
        <translation>PIN при запуске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="149"/>
        <source>No extra protection</source>
        <translation>Без дополнительной защиты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutStatusInfo.cpp" line="159"/>
        <source>This profile can leave this computer, so the system keychain is not offered: a key sealed with one computer&apos;s keychain cannot be opened on another.</source>
        <translation>Этот профиль может покинуть этот компьютер, поэтому системная связка ключей не предлагается: ключ, запечатанный связкой ключей одного компьютера, не может быть открыт на другом.</translation>
    </message>
    <message>
        <source>Inside this profile</source>
        <translation type="vanished">Внутри этого профиля</translation>
    </message>
    <message>
        <source>This computer&apos;s GnuPG keyring</source>
        <translation type="vanished">Связка ключей GnuPG этого компьютера</translation>
    </message>
    <message>
        <source>The package carries none of its own.</source>
        <translation type="vanished">Пакет не несёт собственной связки ключей.</translation>
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
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="128"/>
        <source>How aggressively the application protects your secrets. Higher levels cost some performance.</source>
        <translation>Насколько агрессивно приложение защищает ваши секреты. Более высокие уровни снижают производительность.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="130"/>
        <source>Secure Level:</source>
        <translation>Уровень защиты:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="136"/>
        <source>No extra protection (default)</source>
        <translation>Без дополнительной защиты (по умолчанию)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="139"/>
        <source>System keychain</source>
        <translation>Системная связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="141"/>
        <source>PIN at startup</source>
        <translation>PIN при запуске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="144"/>
        <source>How the application key file is protected on disk. The system keychain keeps a secret on this computer and never asks you for a password; a PIN is asked for each time the application starts.</source>
        <translation>Как файл ключа приложения защищается на диске. Системная связка ключей хранит секрет на этом компьютере и никогда не запрашивает пароль; PIN запрашивается при каждом запуске приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="148"/>
        <source>Change PIN…</source>
        <translation>Изменить PIN…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="153"/>
        <source>Application Key Protection:</source>
        <translation>Защита ключа приложения:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="156"/>
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
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="164"/>
        <source>Diagnostics</source>
        <translation>Диагностика</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="171"/>
        <source>Trace</source>
        <translation>Трассировка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="172"/>
        <source>Debug</source>
        <translation>Отладка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="173"/>
        <source>Info</source>
        <translation>Информация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="174"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="176"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="178"/>
        <source>Fatal</source>
        <translation>Критическая</translation>
    </message>
    <message>
        <source>The least severe message that still gets written to the log. Debug is the most detailed and writes the most to disk.</source>
        <translation type="vanished">Наименее критичное сообщение, которое всё ещё записывается в журнал. Отладка — самый подробный уровень, записывающий больше всего данных на диск.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="180"/>
        <source>The least severe message that still gets written to the log. Trace is the most detailed and writes the most to disk; it adds detailed per-item output from modules that Debug leaves out.</source>
        <translation>Наименее критичное сообщение, которое всё ещё записывается в журнал. Трассировка — самая подробная и записывает больше всего данных на диск; она добавляет детальный по элементам вывод из модулей, которые уровень «Отладка» пропускает.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="183"/>
        <source>Log Level:</source>
        <translation>Уровень журнала:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="187"/>
        <source> entries</source>
        <translation>записи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="189"/>
        <source>How many recent log messages are kept in memory for crash reports and the log viewer. Larger values use more memory.</source>
        <translation>Сколько последних сообщений журнала хранится в памяти для отчетов о сбоях и просмотра журнала. Большие значения потребляют больше памяти.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="191"/>
        <source>Log Ring Buffer:</source>
        <translation>Кольцевой буфер журнала:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="194"/>
        <source>These settings are read once while the application starts, so a change only takes effect after a restart.</source>
        <translation>Эти параметры считываются один раз при запуске приложения, поэтому изменения вступают в силу только после перезапуска.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="200"/>
        <source>Some settings on this page are fixed by the ENV.ini file next to the application and cannot be changed here. Edit that file to change them.</source>
        <translation>Некоторые параметры на этой странице задаются файлом ENV.ini рядом с приложением и не могут быть изменены здесь. Отредактируйте этот файл, чтобы изменить их.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="249"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="517"/>
        <source>System Keychain Unavailable</source>
        <translation>Системная связка ключей недоступна</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="250"/>
        <source>The system credential store could not be used, so the application key cannot be protected with it.</source>
        <translation>Не удалось использовать системное хранилище учетных данных, поэтому ключ приложения не может быть с его помощью защищен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="253"/>
        <source>On Linux this needs a running secret service, such as GNOME Keyring or KWallet, and it must be unlocked.</source>
        <translation>В Linux для этого требуется работающий сервис секретов, например, GNOME Keyring или KWallet, и он должен быть разблокирован.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="283"/>
        <source>Turn Off Weekly Key Rotation?</source>
        <translation>Отключить еженедельную смену ключа?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="284"/>
        <source>At the %1 level the application saves your data with a key that changes every week. Choosing a lower level stops loading those keys, so anything saved while this level was on can no longer be read and is deleted after a short grace period.</source>
        <translation>На уровне %1 приложение сохраняет ваши данные с помощью ключа, который меняется каждую неделю. Выбор более низкого уровня останавливает загрузку этих ключей, поэтому все, что было сохранено, пока этот уровень был включен, больше не может быть прочитано и будет удалено после короткого льготного периода.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="290"/>
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
        <source>This is a temporary profile. Its key is held in memory only and is destroyed when the window closes, so there is nothing to protect.</source>
        <translation>Это временный профиль. Его ключ хранится только в памяти и уничтожается при закрытии окна, поэтому защищать нечего.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="418"/>
        <source>Not available for a profile file: the file is meant to be opened on another computer, possibly running another operating system, where a secret stored in this computer&apos;s credential store cannot be read. Use a PIN, or no protection.</source>
        <translation>Недоступно для файла профиля: файл предназначен для открытия на другом компьютере, возможно, с другой операционной системой, где секрет, хранящийся в хранилище учётных данных этого компьютера, не может быть прочитан. Используйте PIN или не используйте защиту.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="423"/>
        <source>Not available in portable mode: a portable installation must not depend on secrets stored on one particular computer.</source>
        <translation>Недоступно в портативном режиме: портативная установка не должна зависеть от секретов, хранящихся на одном конкретном компьютере.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="431"/>
        <source>No system credential store is available on this computer. On Linux this needs a running secret service, such as GNOME Keyring or KWallet, and the libsecret client library (libsecret-1-0 on Debian and Ubuntu, libsecret on Fedora).</source>
        <translation>На этом компьютере нет системного хранилища учётных данных. В Linux для этого требуется работающая служба секретов, например GNOME Keyring или KWallet, и клиентская библиотека libsecret (libsecret-1-0 в Debian и Ubuntu, libsecret в Fedora).</translation>
    </message>
    <message>
        <source>No system credential store is available on this computer. On Linux this needs a running secret service, such as GNOME Keyring or KWallet.</source>
        <translation type="vanished">На этом компьютере нет системного хранилища учётных данных. В Linux для этого требуется работающая служба секретов, например GNOME Keyring или KWallet.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="518"/>
        <source>The application key could not be protected with the system keychain, so it has been left exactly as it was.</source>
        <translation>Не удалось защитить ключ приложения с помощью системной связки ключей, поэтому он остался в исходном состоянии.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="522"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="572"/>
        <source>Application Key Error</source>
        <translation>Ошибка ключа приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="523"/>
        <source>The application key could not be re-protected: %1.</source>
        <translation>Не удалось повторно защитить ключ приложения: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="526"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="574"/>
        <source>It has been left exactly as it was, so nothing has been lost.</source>
        <translation>Он остался в исходном состоянии, поэтому ничего не потеряно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="555"/>
        <source>The current PIN is not correct.</source>
        <translation>Текущий PIN-код неверен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="566"/>
        <source>PIN Changed</source>
        <translation>PIN-код изменён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="567"/>
        <source>The application PIN has been changed.</source>
        <translation>PIN-код приложения был изменён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAdvanced.cpp" line="573"/>
        <source>The PIN could not be changed: %1.</source>
        <translation>Не удалось изменить PIN-код: %1.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::AppKeyPinDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="62"/>
        <source>Unlock Application Key</source>
        <translation>Разблокировать ключ приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="65"/>
        <source>Change Application PIN</source>
        <translation>Изменить PIN приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="68"/>
        <source>Set an Application PIN</source>
        <translation>Установить PIN приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="74"/>
        <source>This PIN encrypts the application key on disk. You will be asked for it every time the application starts.</source>
        <translation>Этот PIN шифрует ключ приложения на диске. Он будет запрашиваться при каждом запуске приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="77"/>
        <source>This application&apos;s key is protected by a PIN. Enter it to continue.</source>
        <translation>Ключ этого приложения защищён PIN. Введите его для продолжения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="82"/>
        <source>Current PIN</source>
        <translation>Текущий PIN</translation>
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
        <translation>Новый PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="87"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="88"/>
        <source>Show PIN</source>
        <translation>Показать PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="89"/>
        <source>Strength</source>
        <translation>Сложность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="112"/>
        <source>If you forget this PIN, everything the application has encrypted becomes permanently unreadable. There is no recovery.</source>
        <translation>Если вы забудете этот PIN, всё, что зашифровало приложение, станет навсегда недоступным для чтения. Восстановление невозможно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="106"/>
        <source>This PIN cannot be recovered if it is lost.</source>
        <translation>Этот PIN невозможно восстановить в случае утери.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="104"/>
        <source>Use at least %1 characters.</source>
        <translation>Используйте не менее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="92"/>
        <source>Unlock</source>
        <translation>Разблокировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="93"/>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="99"/>
        <source>Quit</source>
        <translation>Выйти</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="122"/>
        <source>Forgot PIN? Reset…</source>
        <translation>Забыли PIN? Сбросить…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="118"/>
        <source>The PIN must be at least %1 characters.</source>
        <translation>PIN должен содержать не менее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="119"/>
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
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="149"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="150"/>
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
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="152"/>
        <source>Toolbar</source>
        <translation>Панель инструментов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="154"/>
        <source>Icon Size</source>
        <translation>Размер значков</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="155"/>
        <source>small</source>
        <translation>маленький</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="156"/>
        <source>medium</source>
        <translation>средний</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="157"/>
        <source>large</source>
        <translation>большой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="159"/>
        <source>Icon Style</source>
        <translation>Стиль значков</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="160"/>
        <source>just text</source>
        <translation>только текст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="161"/>
        <source>just icons</source>
        <translation>только значки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="162"/>
        <source>text and icons</source>
        <translation>текст и значки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="164"/>
        <source>Actions</source>
        <translation>Действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="168"/>
        <source>On click</source>
        <translation>По нажатию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="170"/>
        <source>What a click on the Encrypt and Decrypt toolbar buttons does. The other ticked actions of the row stay in the list shown on right-click.</source>
        <translation>Определяет, что происходит при нажатии на кнопки панели инструментов «Зашифровать» и «Расшифровать». Остальные отмеченные действия строки остаются в списке, отображаемом при щелчке правой кнопкой мыши.</translation>
    </message>
    <message>
        <source>IM actions turn the text into one compact line that is safe to paste into an instant messenger.</source>
        <translation type="vanished">Действия IM преобразуют текст в одну компактную строку, которую можно безопасно вставить в мессенджер.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="187"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="199"/>
        <source>Font Family</source>
        <translation>Семейство шрифтов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="189"/>
        <source>Tab Size</source>
        <translation>Размер табуляции</translation>
    </message>
    <message>
        <source>Text Direction</source>
        <translation type="vanished">Направление текста</translation>
    </message>
    <message>
        <source>Automatic</source>
        <translation type="vanished">Автоматически</translation>
    </message>
    <message>
        <source>Left-to-Right</source>
        <translation type="vanished">Слева направо</translation>
    </message>
    <message>
        <source>Right-to-Left</source>
        <translation type="vanished">Справа налево</translation>
    </message>
    <message>
        <source>Which way the message text runs. Automatic follows the first letter of the text, so a message written in Arabic, Hebrew or Persian reads from the right on its own. Applies to the editor tabs and to the status panel.</source>
        <translation type="vanished">Направление текста сообщения. В режиме «Автоматически» направление определяется по первой букве текста, поэтому сообщение на арабском, иврите или персидском автоматически читается справа налево. Применяется к вкладкам редактора и панели состояния.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="191"/>
        <source>Show all fonts</source>
        <translation>Показать все шрифты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="193"/>
        <source>Also offer proportional fonts for both surfaces below. They line up best with a monospaced one, but scripts such as Arabic are only shaped correctly by a font that covers them, and the monospaced font shipped with GpgFrontend covers Latin, Greek and Cyrillic only.</source>
        <translation>Также предлагать пропорциональные шрифты для обеих поверхностей ниже. Они лучше всего сочетаются с моноширинным шрифтом, но такие письменности, как арабская, правильно отображаются только шрифтом, который их поддерживает, а моноширинный шрифт, поставляемый с GpgFrontend, охватывает только латиницу, греческий и кириллицу.</translation>
    </message>
    <message>
        <source>Also offer proportional fonts for both surfaces below. They line up best with a monospaced one, but scripts such as Arabic are only shaped correctly by a font that covers them.</source>
        <translation type="vanished">Также предлагает пропорциональные шрифты для обеих областей ниже. Лучше всего они выравниваются с моноширинным шрифтом, но такие письменности, как арабская, правильно отображаются только шрифтом, который их поддерживает.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="198"/>
        <source>Status Panel</source>
        <translation>Панель состояния</translation>
    </message>
    <message>
        <source>Style</source>
        <translation type="vanished">Стиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="188"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="200"/>
        <source>Font Size</source>
        <translation>Размер шрифта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsAppearance.cpp" line="186"/>
        <source>Text Editor</source>
        <translation>Текстовый редактор</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::BuildInfoTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="349"/>
        <source>GpgFrontend:</source>
        <translation>GpgFrontend:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="350"/>
        <source>Qt:</source>
        <translation>Qt:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="351"/>
        <source>GPGME:</source>
        <translation>GPGME:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="352"/>
        <source>Assuan:</source>
        <translation>Assuan:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="353"/>
        <source>Libarchive:</source>
        <translation>Libarchive:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="354"/>
        <source>OpenSSL:</source>
        <translation>OpenSSL:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="355"/>
        <source>Sodium:</source>
        <translation>Sodium:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="356"/>
        <source>Git Branch:</source>
        <translation>Ветка Git:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="357"/>
        <source>Git Commit:</source>
        <translation>Коммит Git:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="358"/>
        <source>Built at:</source>
        <translation>Собрано:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="362"/>
        <source>Copy Build Information</source>
        <translation>Копировать информацию о сборке</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="369"/>
        <source>Build Information</source>
        <translation>Информация о сборке</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ChoosePage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="406"/>
        <source>Choose a guide</source>
        <translation>Выберите руководство</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="407"/>
        <source>Pick a topic if you want to learn the basics first.</source>
        <translation>Выберите тему, если хотите сначала изучить основы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="409"/>
        <source>Generate a new Key Pair</source>
        <translation>Создать новую пару ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="410"/>
        <source>Learn how to create your own key pairs.</source>
        <translation>Узнайте, как создавать собственные пары ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="413"/>
        <source>Text operations</source>
        <translation>Текстовые операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="414"/>
        <source>Learn how to encrypt, decrypt, sign, and verify text messages.</source>
        <translation>Узнайте, как шифровать, расшифровывать, подписывать и проверять текстовые сообщения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="417"/>
        <source>File operations</source>
        <translation>Файловые операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="418"/>
        <source>Learn how to encrypt, decrypt, sign, and verify files securely.</source>
        <translation>Узнайте, как безопасно шифровать, расшифровывать, подписывать и проверять файлы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="421"/>
        <source>View key pair information</source>
        <translation>Просмотр информации о паре ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="422"/>
        <source>Learn how to inspect key details, user IDs, fingerprints, and key capabilities.</source>
        <translation>Узнайте, как просматривать сведения о ключе, идентификаторы пользователей, отпечатки и возможности ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="426"/>
        <source>You can also skip these guides and start using GpgFrontend directly.</source>
        <translation>Вы также можете пропустить эти руководства и сразу начать использовать GpgFrontend.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ConclusionPage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="468"/>
        <source>Ready to use</source>
        <translation>Готово к использованию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="470"/>
        <source>GpgFrontend is ready. You can adjust these options before finishing.</source>
        <translation>GpgFrontend готов к работе. Вы можете изменить эти параметры перед завершением.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="474"/>
        <source>Contact and feedback</source>
        <translation>Контакты и обратная связь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="475"/>
        <source>Report issues, ask questions, or send feedback to help improve GpgFrontend.</source>
        <translation>Сообщайте о проблемах, задавайте вопросы или отправляйте отзывы, чтобы помочь улучшить GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="479"/>
        <source>Submit an issue on GitHub</source>
        <translation>Сообщить о проблеме на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="480"/>
        <source>Use GitHub issues if you want to report a bug or track a technical problem.</source>
        <translation>Используйте GitHub Issues, если хотите сообщить об ошибке или отследить техническую проблему.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="485"/>
        <source>Don&apos;t show this setup wizard again</source>
        <translation>Больше не показывать мастер настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="483"/>
        <source>Check for updates on startup</source>
        <translation>Проверять обновления при запуске</translation>
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
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="125"/>
        <source>Zoom out</source>
        <translation>Уменьшить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="128"/>
        <source>Zoom in</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="131"/>
        <source>Fit to window</source>
        <translation>Вписать в окно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/DocViewerDialog.cpp" line="134"/>
        <source>Actual size (100%)</source>
        <translation>Настоящий размер (100%)</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::EncryptionKeysPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="171"/>
        <source>Select Encryption Key(s)</source>
        <translation>Выберите ключ(и) шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="172"/>
        <source>Choose Encryption Key(s) or Subkey(s)</source>
        <translation>Выберите ключ(и) или подключ(и) шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="173"/>
        <source>Select the recipient key(s) or specific encryption subkey(s) to use for this operation. Expand a key to choose a particular subkey.</source>
        <translation>Выберите ключ(и) получателя или конкретный(е) подключ(и) шифрования для использования в этой операции. Разверните ключ, чтобы выбрать конкретный подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="176"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="176"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="176"/>
        <source>No Key Selected</source>
        <translation>Ключ не выбран</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="177"/>
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
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="78"/>
        <source>Export key package</source>
        <translation>Экспортировать пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="133"/>
        <source>Check which keys go into the package, and whether their private keys go with them.</source>
        <translation>Проверьте, какие ключи войдут в пакет и будут ли вместе с ними включены их закрытые ключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="149"/>
        <source>Only export keys with a private key</source>
        <translation>Экспортировать только ключи с закрытым ключом</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="155"/>
        <source>Include private keys</source>
        <translation>Включить закрытые ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="165"/>
        <source>Private keys let anyone who has this package, its key file and the PIN decrypt your messages and sign as you.</source>
        <translation>Закрытые ключи позволяют любому, у кого есть этот пакет, его файл ключа и пароль, расшифровывать ваши сообщения и подписывать их от вашего имени.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="194"/>
        <source>Keys to export</source>
        <translation>Ключи для экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="227"/>
        <source>Name the package and choose where to save it. Its key file is saved beside it unless you choose another place, such as a separate drive.</source>
        <translation>Задайте имя пакета и выберите место для сохранения. Его файл ключа сохраняется рядом с ним, если вы не выберете другое место, например отдельный диск.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="237"/>
        <source>Package name</source>
        <translation>Имя пакета</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="245"/>
        <source>Generate name</source>
        <translation>Сгенерировать имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="247"/>
        <source>Generate a package name</source>
        <translation>Сгенерировать имя пакета</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="263"/>
        <source>No location chosen yet</source>
        <translation>Место ещё не выбрано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="265"/>
        <source>Browse...</source>
        <translation>Обзор...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="272"/>
        <source>Save to</source>
        <translation>Сохранить в</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="272"/>
        <source>Choose where to save the key package</source>
        <translation>Выберите место сохранения пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="274"/>
        <source>Key file</source>
        <translation>Файл ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="274"/>
        <source>Choose where to save the key file</source>
        <translation>Выберите место сохранения файла ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="281"/>
        <source>Keep next to the package</source>
        <translation>Хранить рядом с пакетом</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="338"/>
        <source>Choose a PIN to protect the key file. Importing needs the package, the key file and this PIN, and the PIN cannot be recovered.</source>
        <translation>Выберите PIN для защиты файла ключа. Для импорта требуются пакет, файл ключа и этот PIN, а PIN невозможно восстановить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="357"/>
        <source>PIN to protect the key file</source>
        <translation>PIN для защиты файла ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="467"/>
        <source>Save Key File</source>
        <translation>Сохранить файл ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="468"/>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="510"/>
        <source>All Files</source>
        <translation>Все файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="507"/>
        <source>Export Key Package</source>
        <translation>Экспортировать пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="510"/>
        <source>Key Package</source>
        <translation>Пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="540"/>
        <source>Saved beside the package.</source>
        <translation>Сохранён рядом с пакетом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="541"/>
        <source>Saved in a place of its own.</source>
        <translation>Сохранён в отдельном месте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="571"/>
        <source>Replace File?</source>
        <translation>Заменить файл?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="572"/>
        <source>%1 already exists. Do you want to replace it?</source>
        <translation>%1 уже существует. Заменить его?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="606"/>
        <source>Exporting</source>
        <translation>Экспорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="629"/>
        <source>Export Failed</source>
        <translation>Не удалось экспортировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="630"/>
        <source>The key package could not be written.</source>
        <translation>Не удалось сохранить пакет ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="633"/>
        <source>Check that both folders still exist and can be written to, then try again. Your choices are kept.</source>
        <translation>Убедитесь, что обе папки по-прежнему существуют и доступны для записи, затем повторите попытку. Сделанный выбор сохранён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="645"/>
        <source>Key Package Exported</source>
        <translation>Пакет ключей экспортирован</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="646"/>
        <source>The key package and its key file have been saved.</source>
        <translation>Пакет ключей и его файл ключа сохранены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="650"/>
        <source>Keep the key file and the PIN apart from the package, and delete both files once the transfer is done.</source>
        <translation>Храните файл ключа и PIN отдельно от пакета и удалите оба файла после завершения передачи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="652"/>
        <source>Done</source>
        <translation>Готово</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="654"/>
        <source>Show in Folder</source>
        <translation>Показать в папке</translation>
    </message>
    <message>
        <source>Export Key Package Passphrase</source>
        <translation type="vanished">Пароль пакета ключей для экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="468"/>
        <source>Key File</source>
        <translation>Файл ключа</translation>
    </message>
    <message>
        <source>Error</source>
        <translation type="vanished">Ошибка</translation>
    </message>
    <message>
        <source>Forbidden</source>
        <translation type="vanished">Запрещено</translation>
    </message>
    <message>
        <source>Please select an output path before exporting.</source>
        <translation type="vanished">Выберите путь вывода перед экспортом.</translation>
    </message>
    <message>
        <source>Please generate a password to protect your key before exporting, it is very important. Don&apos;t forget to back up your password in a safe place.</source>
        <translation type="vanished">Сгенерируйте пароль для защиты ключа перед экспортом — это очень важно. Не забудьте сохранить пароль в надёжном месте.</translation>
    </message>
    <message>
        <source>No key is suitable to export.</source>
        <translation type="vanished">Нет подходящих ключей для экспорта.</translation>
    </message>
    <message>
        <source>Enter PIN</source>
        <translation type="vanished">Введите PIN</translation>
    </message>
    <message>
        <source>Please enter PIN to protect the Key:</source>
        <translation type="vanished">Введите PIN для защиты ключа:</translation>
    </message>
    <message>
        <source>Generating</source>
        <translation type="vanished">Генерация</translation>
    </message>
    <message>
        <source>Success</source>
        <translation type="vanished">Успешно</translation>
    </message>
    <message>
        <source>But the key file cannot be leaked under any circumstances. Please delete the Key Package and key file as soon as possible after completing the transfer operation.</source>
        <translation type="vanished">Файл ключа ни в коем случае не должен быть раскрыт. Удалите пакет ключей и файл ключа как можно скорее после завершения передачи.</translation>
    </message>
    <message>
        <source>The Key Package has been successfully generated and has been protected by encryption algorithms(AES-256-GCM). You can safely transfer your Key Package.</source>
        <translation type="vanished">Пакет ключей успешно сгенерирован и защищён алгоритмом шифрования (AES-256-GCM). Вы можете безопасно передать свой пакет ключей.</translation>
    </message>
    <message>
        <source>Key Package Name</source>
        <translation type="vanished">Имя пакета ключей</translation>
    </message>
    <message>
        <source>Output Path</source>
        <translation type="vanished">Путь вывода</translation>
    </message>
    <message>
        <source>Passphrase</source>
        <translation type="vanished">Парольная фраза</translation>
    </message>
    <message>
        <source>Tips: You can use Key Package to safely and conveniently transfer your public and private keys between devices.</source>
        <translation type="vanished">Подсказка: вы можете использовать пакет ключей для безопасной и удобной передачи открытых и закрытых ключей между устройствами.</translation>
    </message>
    <message>
        <source>Generate and Save Passphrase</source>
        <translation type="vanished">Сгенерировать и сохранить парольную фразу</translation>
    </message>
    <message>
        <source>Generate Key Package Name</source>
        <translation type="vanished">Сгенерировать имя пакета ключей</translation>
    </message>
    <message>
        <source>Select Output Path</source>
        <translation type="vanished">Выберите путь вывода</translation>
    </message>
    <message>
        <source>Include secret key (Think twice before acting)</source>
        <translation type="vanished">Включить секретный ключ (подумайте дважды перед действием)</translation>
    </message>
    <message>
        <source>Exclude keys that do not have a private key</source>
        <translation type="vanished">Исключить ключи без закрытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/ExportKeyPackageDialog.cpp" line="88"/>
        <source>Export As Key Package</source>
        <translation>Экспортировать как пакет ключей</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FileOperaOutcome</name>
    <message>
        <location filename="../../../../src/ui/main_window/FileOperaOutcome.cpp" line="49"/>
        <source>The staged output to save was missing.</source>
        <translation>Подготовленный для сохранения выходной файл отсутствовал.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/FileOperaOutcome.cpp" line="53"/>
        <source>The existing file could not be moved aside to be replaced.</source>
        <translation>Не удалось переместить существующий файл в сторону для замены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/FileOperaOutcome.cpp" line="58"/>
        <source>The output could not be moved into place.</source>
        <translation>Не удалось переместить выходной файл на место.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/FileOperaOutcome.cpp" line="66"/>
        <source>The staged output could not be removed.</source>
        <translation>Не удалось удалить подготовленный выходной файл.</translation>
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
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="109"/>
        <source>New Folder</source>
        <translation>Новая папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="110"/>
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
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="207"/>
        <source>Batch mode is enabled. No file is selected.</source>
        <translation>Включен пакетный режим. Файлы не выбраны.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="208"/>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="227"/>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="266"/>
        <source>Enable batch mode to select multiple files.</source>
        <translation>Включите пакетный режим для выбора нескольких файлов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="213"/>
        <source>%1 item(s) selected.</source>
        <translation>Выбрано элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="226"/>
        <source>Batch mode is enabled. Multiple files can be selected.</source>
        <translation>Включен пакетный режим. Можно выбрать несколько файлов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="236"/>
        <source>Filter</source>
        <translation>Фильтр</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="238"/>
        <source>List only the files and folders whose name contains this text. The filter applies to the current folder and is cleared when you open another one.</source>
        <translation>Показывать только файлы и папки, в имени которых содержится этот текст. Фильтр применяется к текущей папке и сбрасывается при открытии другой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="259"/>
        <source>Go to the parent folder (Backspace)</source>
        <translation>Перейти в родительскую папку (Backspace)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="260"/>
        <source>Read this folder from disk again</source>
        <translation>Обновить содержимое этой папки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="262"/>
        <source>Go to a mounted volume or removable drive</source>
        <translation>Перейти к смонтированному тому или сменному диску</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="264"/>
        <source>Create items and choose what the list shows</source>
        <translation>Создавайте элементы и выбирайте, что отображается в списке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="294"/>
        <source>Entries listed in this folder, and how many of them are selected. Entries hidden by the filter are not counted.</source>
        <translation>Записи в этой папке и количество выбранных. Записи, скрытые фильтром, не учитываются.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="297"/>
        <source>Space still available on the volume holding this folder.</source>
        <translation>Свободное место на томе, содержащем эту папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="432"/>
        <source>Profile Workspace</source>
        <translation>Рабочее пространство профиля</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="480"/>
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
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="483"/>
        <source>%1 selected</source>
        <translation>%1 выбрано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="497"/>
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
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="316"/>
        <source>The folder does not exist or cannot be opened.</source>
        <translation>Папка не существует или не может быть открыта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="465"/>
        <source>No Available Volumes</source>
        <translation>Нет доступных томов</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FilePathBar</name>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="134"/>
        <source>Type a folder path, e.g. ~/Documents</source>
        <translation>Введите путь к папке, например ~/Документы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="136"/>
        <source>Type a folder path and press Enter. &quot;~&quot; stands for your home folder, and a relative path is resolved against the current one. Press Escape to go back to the path buttons.</source>
        <translation>Введите путь к папке и нажмите Enter. &quot;~&quot; обозначает вашу домашнюю папку, а относительный путь разрешается относительно текущей. Нажмите Escape, чтобы вернуться к кнопкам пути.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="162"/>
        <source>Click a folder in the path to go there, or click the edit button to type a path (Ctrl+L).</source>
        <translation>Нажмите на папку в пути, чтобы перейти туда, или нажмите кнопку редактирования, чтобы ввести путь (Ctrl+L).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="418"/>
        <source>Type a path instead (Ctrl+L)</source>
        <translation>Ввести путь вместо этого (Ctrl+L)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="438"/>
        <source>Show parent folders</source>
        <translation>Показать родительские папки</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FileSystemItemNameDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="68"/>
        <source>Rename Folder</source>
        <translation>Переименовать папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="68"/>
        <source>Rename File</source>
        <translation>Переименовать файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="70"/>
        <source>New Folder</source>
        <translation>Новая папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="70"/>
        <source>New File</source>
        <translation>Новый файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="81"/>
        <source>Rename this folder</source>
        <translation>Переименовать эту папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="81"/>
        <source>Rename this file</source>
        <translation>Переименовать этот файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="84"/>
        <source>Create a new folder</source>
        <translation>Создать новую папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="84"/>
        <source>Create a new empty file</source>
        <translation>Создать новый пустой файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="97"/>
        <source>Location: %1</source>
        <translation>Расположение: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="101"/>
        <source>Folder name</source>
        <translation>Имя папки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="102"/>
        <source>File name, e.g. notes.txt</source>
        <translation>Имя файла, например notes.txt</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="109"/>
        <source>Name:</source>
        <translation>Имя:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="115"/>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="115"/>
        <source>Rename</source>
        <translation>Переименовать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="184"/>
        <source>Enter a folder name.</source>
        <translation>Введите имя папки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="184"/>
        <source>Enter a file name.</source>
        <translation>Введите имя файла.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="187"/>
        <source>This name is reserved.</source>
        <translation>Это имя зарезервировано.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="190"/>
        <source>The name must not contain path separators.</source>
        <translation>Имя не должно содержать разделителей пути.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="193"/>
        <source>This name is reserved by the operating system.</source>
        <translation>Это имя зарезервировано операционной системой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="196"/>
        <source>Enter a name different from the current one.</source>
        <translation>Введите имя, отличное от текущего.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="199"/>
        <source>A file or folder with this name already exists.</source>
        <translation>Файл или папка с таким именем уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="203"/>
        <source>The folder will be renamed in place.</source>
        <translation>Папка будет переименована на месте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="204"/>
        <source>The file will be renamed in place.</source>
        <translation>Файл будет переименован на месте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="210"/>
        <source>The folder will be created in the selected location.</source>
        <translation>Папка будет создана в выбранном расположении.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/FileSystemItemNameDialog.cpp" line="211"/>
        <source>An empty file will be created in the selected location.</source>
        <translation>Пустой файл будет создан в выбранном расположении.</translation>
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
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="591"/>
        <source>Rename</source>
        <translation>Переименовать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="575"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="605"/>
        <source>Calculate Hash</source>
        <translation>Вычислить хеш</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="627"/>
        <source>Compress...</source>
        <translation>Сжать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="328"/>
        <source>Unable to Open File</source>
        <translation>Не удалось открыть файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="253"/>
        <source>This folder cannot be opened</source>
        <translation>Не удалось открыть эту папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="255"/>
        <source>No file matches &quot;%1&quot;</source>
        <translation>Нет файла, соответствующего &quot;%1&quot;</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="256"/>
        <source>This folder is empty</source>
        <translation>Эта папка пуста.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="329"/>
        <source>The file cannot be read.</source>
        <translation>Файл не может быть прочитан.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="339"/>
        <source>Unable to Open Folder</source>
        <translation>Не удалось открыть папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="340"/>
        <source>The folder cannot be opened. Please check permissions.</source>
        <translation>Папка не может быть открыта. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="388"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="597"/>
        <source>Move to Trash</source>
        <translation>Переместить в корзину</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="390"/>
        <source>Move &quot;%1&quot; to Trash?</source>
        <translation>Переместить &quot;%1&quot; в корзину?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="391"/>
        <source>Move %1 selected items to Trash?</source>
        <translation>Переместить выбранные элементы (%1) в корзину?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="413"/>
        <source>Unable to Move to Trash</source>
        <translation>Не удалось переместить в корзину</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="415"/>
        <source>The item &quot;%1&quot; could not be moved to Trash.</source>
        <translation>Элемент &quot;%1&quot; не удалось переместить в корзину.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="417"/>
        <source>%1 item(s) could not be moved to Trash.</source>
        <translation>Не удалось переместить в корзину элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="422"/>
        <source>Moved %1 item(s) to Trash.</source>
        <translation>Элементов перемещено в корзину: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="445"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="453"/>
        <source>Unable to Create Folder</source>
        <translation>Не удалось создать папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="446"/>
        <source>The target folder is not available.</source>
        <translation>Целевая папка недоступна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="454"/>
        <source>The folder could not be created. Please check permissions.</source>
        <translation>Не удалось создать папку. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="462"/>
        <source>Created folder: %1</source>
        <translation>Создана папка: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="483"/>
        <source>Unable to Create File</source>
        <translation>Не удалось создать файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="484"/>
        <source>The file could not be created. Please check permissions.</source>
        <translation>Не удалось создать файл. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="498"/>
        <source>Created file: %1</source>
        <translation>Создан файл: %1</translation>
    </message>
    <message>
        <source>New name:</source>
        <translation type="vanished">Новое имя:</translation>
    </message>
    <message>
        <source>Invalid Name</source>
        <translation type="vanished">Недопустимое имя</translation>
    </message>
    <message>
        <source>The name must not contain path separators.</source>
        <translation type="vanished">Имя не должно содержать разделителей пути.</translation>
    </message>
    <message>
        <source>Name Already Exists</source>
        <translation type="vanished">Имя уже существует</translation>
    </message>
    <message>
        <source>A file or folder with this name already exists.</source>
        <translation type="vanished">Файл или папка с таким именем уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="555"/>
        <source>Unable to Rename</source>
        <translation>Не удалось переименовать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="556"/>
        <source>The file or folder could not be renamed.</source>
        <translation>Не удалось переименовать файл или папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="562"/>
        <source>Renamed &quot;%1&quot; to &quot;%2&quot;.</source>
        <translation>Переименовано из &quot;%1&quot; в &quot;%2&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="577"/>
        <source>Open the file in a GpgFrontend editor tab (Enter).</source>
        <translation>Открыть файл на вкладке редактора GpgFrontend (Enter).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="584"/>
        <source>Open with Default Application</source>
        <translation>Открыть в приложении по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="586"/>
        <source>Hand the item to the application your system uses for it.</source>
        <translation>Передать элемент приложению, которое ваша система использует для него.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="592"/>
        <source>Give the item a new name (F2).</source>
        <translation>Переименовать элемент (F2).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="599"/>
        <source>Move the selected items to the system Trash, where they can still be recovered (Delete).</source>
        <translation>Переместить выбранные элементы в системную корзину, откуда их можно восстановить (Delete).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="607"/>
        <source>Compute checksums of the file and show them on the information board, to compare it against a published value.</source>
        <translation>Вычислить контрольные суммы файла и показать их на информационной панели для сравнения с опубликованным значением.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="613"/>
        <source>Folder</source>
        <translation>Папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="615"/>
        <source>Create a new folder inside the selected folder.</source>
        <translation>Создать новую папку в выбранной папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="620"/>
        <source>Empty File</source>
        <translation>Пустой файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="622"/>
        <source>Create an empty file inside the selected folder.</source>
        <translation>Создать пустой файл в выбранной папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="631"/>
        <source>Copy Path</source>
        <translation>Копировать путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="633"/>
        <source>Copy the full path of the selected items to the clipboard.</source>
        <translation>Скопировать полный путь выбранных элементов в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="638"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="639"/>
        <source>Read this folder from disk again.</source>
        <translation>Перечитать эту папку с диска.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="643"/>
        <source>New</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="646"/>
        <source>Create a new item in the current folder.</source>
        <translation>Создать новый элемент в текущей папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="755"/>
        <source>Calculating</source>
        <translation>Вычисление</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="774"/>
        <source>File Hash Information</source>
        <translation>Информация о хеше файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="828"/>
        <source>Batch mode is enabled. Use Ctrl or Shift to select multiple items.</source>
        <translation>Включен пакетный режим. Используйте Ctrl или Shift для выбора нескольких элементов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="987"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="995"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1025"/>
        <source>Move Failed</source>
        <translation>Ошибка перемещения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="988"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1181"/>
        <source>A file or folder named &quot;%1&quot; already exists in the target folder.</source>
        <translation>Файл или папка с именем &quot;%1&quot; уже существует в целевой папке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="996"/>
        <source>Unable to move &quot;%1&quot;.

The target may be on another volume, or you may not have sufficient permissions.</source>
        <translation>Не удалось переместить &quot;%1&quot;.

Возможно, целевой объект находится на другом томе или у вас недостаточно прав доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1025"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1180"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1194"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1203"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1218"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1226"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1251"/>
        <source>Copy Failed</source>
        <translation>Ошибка копирования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1026"/>
        <source>The target folder is not writable.</source>
        <translation>Целевая папка недоступна для записи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1049"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1072"/>
        <source>The source and target folder are the same.</source>
        <translation>Исходная и целевая папки совпадают.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1073"/>
        <source>Cannot put &quot;%1&quot; into itself or one of its subfolders.</source>
        <translation>Невозможно поместить &quot;%1&quot; в саму себя или одну из своих подпапок.</translation>
    </message>
    <message>
        <source>Cannot move &quot;%1&quot; into itself or one of its subfolders.</source>
        <translation type="vanished">Невозможно переместить &quot;%1&quot; в саму себя или одну из своих подпапок.</translation>
    </message>
    <message>
        <source>Cannot copy &quot;%1&quot; into itself or one of its subfolders.</source>
        <translation type="vanished">Невозможно скопировать &quot;%1&quot; в саму себя или одну из своих подпапок.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1085"/>
        <source>Move Items</source>
        <translation>Перемещение элементов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1088"/>
        <source>Move &quot;%1&quot; to &quot;%2&quot;?</source>
        <translation>Переместить &quot;%1&quot; в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1090"/>
        <source>Move %1 items to &quot;%2&quot;?</source>
        <translation>Переместить %1 элементов в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1094"/>
        <source>Copy Items</source>
        <translation>Копирование элементов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1097"/>
        <source>Copy &quot;%1&quot; to &quot;%2&quot;?</source>
        <translation>Скопировать &quot;%1&quot; в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1099"/>
        <source>Copy %1 items to &quot;%2&quot;?</source>
        <translation>Скопировать %1 элементов в &quot;%2&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1104"/>
        <source>Move</source>
        <translation>Переместить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1104"/>
        <source>Copy</source>
        <translation>Скопировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1146"/>
        <source>Moved %1 item(s).</source>
        <translation>Перемещено элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1147"/>
        <source>Copied %1 item(s).</source>
        <translation>Скопировано элементов: %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1153"/>
        <source>Move Partially Completed</source>
        <translation>Перемещение завершено частично</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1154"/>
        <source>Copy Partially Completed</source>
        <translation>Копирование завершено частично</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1156"/>
        <source>Some items could not be moved. Please check permissions or whether the target is on another volume.</source>
        <translation>Некоторые элементы не удалось переместить. Пожалуйста, проверьте права доступа или находится ли целевой объект на другом томе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1158"/>
        <source>Some items could not be copied. Please check permissions.</source>
        <translation>Некоторые элементы не удалось скопировать. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1195"/>
        <source>Unable to copy &quot;%1&quot;. Please check permissions.</source>
        <translation>Не удалось скопировать &quot;%1&quot;. Пожалуйста, проверьте права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1204"/>
        <source>&quot;%1&quot; is not a regular file or folder.</source>
        <translation>&quot;%1&quot; не является обычным файлом или папкой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1219"/>
        <source>The target folder &quot;%1&quot; already exists.</source>
        <translation>Целевая папка &quot;%1&quot; уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1227"/>
        <source>Unable to create target folder &quot;%1&quot;.</source>
        <translation>Не удалось создать целевую папку &quot;%1&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1252"/>
        <source>Unable to copy &quot;%1&quot;.</source>
        <translation>Не удалось скопировать &quot;%1&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1272"/>
        <source>Path copied to clipboard.</source>
        <translation>Путь скопирован в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1273"/>
        <source>%1 paths copied to clipboard.</source>
        <translation>Пути (%1) скопированы в буфер обмена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1284"/>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1302"/>
        <source>File list refreshed.</source>
        <translation>Список файлов обновлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1292"/>
        <source>Unable to Refresh</source>
        <translation>Не удалось обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="1293"/>
        <source>The current folder no longer exists or cannot be opened.</source>
        <translation>Текущая папка больше не существует или не может быть открыта.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::FindWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/FindWidget.cpp" line="47"/>
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
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="54"/>
        <source>Base</source>
        <translation>Базовый</translation>
    </message>
    <message>
        <source>Module Loading Policy:</source>
        <translation type="vanished">Политика загрузки модулей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="57"/>
        <source>Only Integrated Modules</source>
        <translation>Только встроенные модули</translation>
    </message>
    <message>
        <source>All Modules</source>
        <translation type="vanished">Все модули</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="56"/>
        <source>Module Discovery:</source>
        <translation>Обнаружение модулей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="59"/>
        <source>Also Look For Modules I&apos;ve Added</source>
        <translation>Также искать добавленные мной модули</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="61"/>
        <source>Disable</source>
        <translation>Отключить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="74"/>
        <source>Default Workspace As:</source>
        <translation>Рабочая область по умолчанию:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="75"/>
        <source>File Panel</source>
        <translation>Панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="76"/>
        <source>Text Editor</source>
        <translation>Текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="82"/>
        <source>File Panel Opens At:</source>
        <translation>Панель файлов открывается в</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="93"/>
        <source>Allow only one checked key in the key list.</source>
        <translation>Разрешить только один отмеченный ключ в списке ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="95"/>
        <source>Checking a key unchecks the others, so a recipient from an earlier message is not added to the next one by mistake.</source>
        <translation>Отметка ключа снимает отметки с остальных, поэтому получатель из предыдущего сообщения не будет случайно добавлен к следующему.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="98"/>
        <source>Ask before encrypting to more than one recipient.</source>
        <translation>Спрашивать перед шифрованием для нескольких получателей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="100"/>
        <source>Allow undoing encrypt, decrypt and other operations in the text editor.</source>
        <translation>Разрешить отмену шифрования, расшифровки и других операций в текстовом редакторе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="103"/>
        <source>Undo keeps the previous text in memory. After encrypting, that is the plaintext.</source>
        <translation>Отмена хранит предыдущий текст в памяти. После шифрования это открытый текст.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="108"/>
        <source>Warn about keys expiring within:</source>
        <translation>Предупреждать об истечении ключей через:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="109"/>
        <source> days</source>
        <translation>дней</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="187"/>
        <source>Profile Workspace</source>
        <translation>Рабочая область профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="193"/>
        <source>Home Folder</source>
        <translation>Домашняя папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="196"/>
        <source>Working Directory</source>
        <translation>Рабочий каталог</translation>
    </message>
    <message>
        <source>Use home path as the default path for FilePanel</source>
        <translation type="vanished">Использовать домашний каталог как путь по умолчанию для панели файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="85"/>
        <source>Cache text editor contents.</source>
        <translation>Кэшировать содержимое текстового редактора.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="119"/>
        <source>System Default follows your operating system. Choose a language here to override it. GpgFrontend restarts automatically when you change it.</source>
        <translation>По умолчанию в системе следует за вашей операционной системой. Выберите здесь язык, чтобы переопределить его. GpgFrontend перезапустится автоматически при его изменении.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="72"/>
        <source>Operation</source>
        <translation>Операция</translation>
    </message>
    <message>
        <source>Default Engine:</source>
        <translation type="vanished">Движок по умолчанию:</translation>
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
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="88"/>
        <source>Import files dropped on the Key List without confirmation.</source>
        <translation>Импортировать файлы, перетащенные в список ключей, без подтверждения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="91"/>
        <source>Remember which keys are checked in the key list between restarts.</source>
        <translation>Запоминать, какие ключи отмечены в списке ключей между перезапусками.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="106"/>
        <source>Use Binary Mode for File Operations</source>
        <translation>Использовать бинарный режим для файловых операций</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="111"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="118"/>
        <source>NOTE</source>
        <translation>ПРИМЕЧАНИЕ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="118"/>
        <source>: </source>
        <translation>: </translation>
    </message>
    <message>
        <source>GpgFrontend will restart automatically if you change the language!</source>
        <translation type="vanished">GpgFrontend автоматически перезапустится при смене языка!</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="123"/>
        <source>Data</source>
        <translation>Данные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="125"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="157"/>
        <source>Clear All Data Objects (Total Size: %1)</source>
        <translation>Очистить все объекты данных (общий размер: %1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="141"/>
        <source>Reveal in File Explorer</source>
        <translation>Показать в проводнике</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="149"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGeneral.cpp" line="150"/>
        <source>Are you sure you want to clear all data objects?
This will result in the loss of all cached form positions, statuses, key servers, etc.</source>
        <translation>Вы уверены, что хотите очистить все объекты данных?
Это приведёт к потере всех сохранённых позиций форм, статусов, серверов ключей и т.д.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GeneralWizard</name>
    <message>
        <location filename="../../../../src/ui/dialog/GeneralWizard.cpp" line="77"/>
        <source>Back</source>
        <translation>Назад</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/GeneralWizard.cpp" line="78"/>
        <source>Continue</source>
        <translation>Продолжить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/GeneralWizard.cpp" line="88"/>
        <source>Step %1 of %2</source>
        <translation>Шаг %1 из %2</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GenerateCardKeyDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="60"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="61"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="62"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="63"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="64"/>
        <source>Non Expire</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="66"/>
        <source>Warning: This Operation will overwrite keys which exist on card!</source>
        <translation>Предупреждение: эта операция перезапишет ключи, находящиеся на карте!</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="67"/>
        <source>Generate</source>
        <translation>Сгенерировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="84"/>
        <source>Card Key Generation</source>
        <translation>Генерация ключей карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="103"/>
        <source>Name must not be empty.</source>
        <translation>Имя не должно быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="109"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Имя и комментарий не должны содержать символы &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; или управляющие символы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="115"/>
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
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="158"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="159"/>
        <source>Card key generation completed successfully.</source>
        <translation>Генерация ключей карты успешно завершена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/GenerateCardKeyDialog.cpp" line="165"/>
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
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="51"/>
        <source>General</source>
        <translation>Общие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="52"/>
        <source>Advanced</source>
        <translation>Расширенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="54"/>
        <source>Enable GpgME Debug Log</source>
        <translation>Включить отладочный журнал GpgME</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="56"/>
        <source>Enable verbose GpgME logs for troubleshooting. This may include technical details about GnuPG operations.</source>
        <translation>Включить подробные журналы GpgME для устранения неполадок. Это может включать технические детали операций GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="59"/>
        <source>Use Custom GnuPG</source>
        <translation>Использовать пользовательский GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="60"/>
        <source>Select GnuPG Path</source>
        <translation>Выбрать путь к GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="62"/>
        <source>Terminate GnuPG background processes on exit</source>
        <translation>Завершать фоновые процессы GnuPG при выходе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="64"/>
        <source>This may affect other applications that are using GnuPG.</source>
        <translation>Это может повлиять на другие приложения, использующие GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="66"/>
        <source>Forbid all GnuPG network connection.</source>
        <translation>Запретить все сетевые соединения GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="71"/>
        <source>Clear password cache on exit</source>
        <translation>Очищать кеш паролей при выходе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="73"/>
        <source>Ask gpg-agent to forget all cached passphrases when GpgFrontend closes.</source>
        <translation>Запрашивать у gpg-agent забыть все кешированные парольные фразы при закрытии GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="79"/>
        <source>Select the directory that contains the &quot;gpgconf&quot; executable.</source>
        <translation>Выберите каталог, содержащий исполняемый файл «gpgconf».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="107"/>
        <source>Open Directory</source>
        <translation>Открыть каталог</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="166"/>
        <source>Maintenance</source>
        <translation>Обслуживание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="170"/>
        <source>These operations take effect immediately and are not undone by cancelling this dialog. Restarting components briefly interrupts any in-flight GnuPG operation.</source>
        <translation>Эти операции вступают в силу немедленно и не отменяются при отмене этого диалога. Перезапуск компонентов ненадолго прерывает любую выполняющуюся операцию GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="177"/>
        <source>Clear Password Cache</source>
        <translation>Очистить кэш паролей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="178"/>
        <source>Clear Password Cache of GnuPG</source>
        <translation>Очистить кэш паролей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="182"/>
        <source>Clear password cache successfully</source>
        <translation>Кэш паролей успешно очищен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="183"/>
        <source>Failed to clear password cache of GnuPG</source>
        <translation>Не удалось очистить кэш паролей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="187"/>
        <source>Reload Components</source>
        <translation>Перезагрузить компоненты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="188"/>
        <source>Reload All GnuPG&apos;s Components</source>
        <translation>Перезагрузить все компоненты GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="192"/>
        <source>Reload all the GnuPG&apos;s components successfully</source>
        <translation>Все компоненты GnuPG успешно перезагружены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="193"/>
        <source>Failed to reload all or one of the GnuPG&apos;s component(s)</source>
        <translation>Не удалось перезагрузить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="197"/>
        <source>Restart Components</source>
        <translation>Перезапустить компоненты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="198"/>
        <source>Restart All GnuPG&apos;s Components</source>
        <translation>Перезапустить все компоненты GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="202"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="203"/>
        <source>Are you sure you want to restart all of GnuPG&apos;s components?
Any GnuPG operation still running will be interrupted.</source>
        <translation>Вы уверены, что хотите перезапустить все компоненты GnuPG?
Любая выполняющаяся операция GnuPG будет прервана.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="210"/>
        <source>Restart all the GnuPG&apos;s components successfully</source>
        <translation>Все компоненты GnuPG успешно перезапущены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="211"/>
        <source>Failed to restart all or one of the GnuPG&apos;s component(s)</source>
        <translation>Не удалось перезапустить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="235"/>
        <source>GnuPG Unavailable</source>
        <translation>GnuPG недоступен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="239"/>
        <source>GnuPG cannot start: this key database&apos;s folder path is too long for GnuPG&apos;s agent socket. Choose a key database in a shorter path under Settings, Key Databases.</source>
        <translation>GnuPG не может запуститься: путь к папке этой базы данных ключей слишком длинный для сокета агента GnuPG. Выберите базу данных ключей с более коротким путём в Настройках, Базы данных ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="284"/>
        <source>Successful Operation</source>
        <translation>Операция выполнена успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="286"/>
        <source>Failed Operation</source>
        <translation>Неудавшаяся операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="383"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="390"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="404"/>
        <source>Illegal GnuPG Path</source>
        <translation>Недопустимый путь к GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="384"/>
        <source>Target GnuPG Path is not an exists readable directory.</source>
        <translation>Целевой путь к GnuPG не является существующим доступным для чтения каталогом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="391"/>
        <source>Target GnuPG Path is not an absolute path.</source>
        <translation>Целевой путь к GnuPG не является абсолютным путем.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsGnuPG.cpp" line="405"/>
        <source>Target GnuPG Path contains no &quot;gpgconf&quot; executable.</source>
        <translation>Целевой путь к GnuPG не содержит исполняемого файла «gpgconf».</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GpgFrontendApplication</name>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="80"/>
        <source>Standard Exception Thrown</source>
        <translation>Выброшено стандартное исключение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="81"/>
        <source>Oops, an standard exception was thrown during the running of the program. This is not a serious problem, it may be the negligence of the programmer, please report this problem if you can.</source>
        <translation>Произошло стандартное исключение во время работы программы. Это не серьёзная проблема, возможно это недосмотр разработчика. Пожалуйста, сообщите об этой проблеме, если можете.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="89"/>
        <source>Unhandled Exception Thrown</source>
        <translation>Выброшено необработанное исключение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendApplication.cpp" line="90"/>
        <source>Oops, an unhandled exception was thrown during the running of the program. This is not a serious problem, it may be the negligence of the programmer, please report this problem if you can.</source>
        <translation>Произошло необработанное исключение во время работы программы. Это не серьёзная проблема, возможно это недосмотр разработчика. Пожалуйста, сообщите об этой проблеме, если можете.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::GpgOperaHelper</name>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="220"/>
        <source>Operation Not Supported</source>
        <translation>Операция не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="224"/>
        <source>Operation Cancelled</source>
        <translation>Операция отменена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="228"/>
        <source>Critical Error</source>
        <translation>Критическая ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="198"/>
        <source>The operation stopped before the engine returned a result.</source>
        <translation>Операция остановилась до того, как движок вернул результат.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="211"/>
        <source>The engine returned no usable result.</source>
        <translation>Движок не вернул пригодного результата.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GpgOperaHelper.cpp" line="836"/>
        <source>Another operation is still running. Please wait for it to finish.</source>
        <translation>Другая операция всё ещё выполняется. Пожалуйста, дождитесь её завершения.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InfoBoardReport</name>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="169"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="588"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1398"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1401"/>
        <source>Result</source>
        <translation>Результат</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="243"/>
        <source>Applied to the current document</source>
        <translation>Применено к текущему документу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="246"/>
        <source>Not applied to the document</source>
        <translation>Не применено к документу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="250"/>
        <source>Written</source>
        <translation>Записано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="252"/>
        <source>Written to %1</source>
        <translation>Записано в %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="256"/>
        <source>Not written: saving failed</source>
        <translation>Не записано: сохранение не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="259"/>
        <source>Not written: saving stopped before this file</source>
        <translation>Не записано: сохранение остановлено до этого файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="263"/>
        <source>Not saved: the staged output was removed</source>
        <translation>Не сохранено: промежуточный вывод удалён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="267"/>
        <source>Not saved: the staged output could not be removed</source>
        <translation>Не сохранено: не удалось удалить промежуточный вывод</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="271"/>
        <source>Withheld: a failed result&apos;s output is not used</source>
        <translation>Удержано: вывод неудачного результата не используется</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="275"/>
        <source>Not produced</source>
        <translation>Не сформировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="287"/>
        <source>apply to document</source>
        <translation>применить к документу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="290"/>
        <source>remove staged output</source>
        <translation>Удалить промежуточный результат</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="294"/>
        <source>save file</source>
        <translation>Сохранить файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="304"/>
        <source>%1 call</source>
        <translation>Вызов %1</translation>
    </message>
    <message>
        <source> (code %1)</source>
        <translation type="vanished">(код %1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="330"/>
        <source>The signer&apos;s public key is not in this key database, so the signature could not be checked.</source>
        <translation>Открытый ключ подписанта отсутствует в этой базе данных ключей, поэтому подпись не удалось проверить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="335"/>
        <source>The signature does not match the data.</source>
        <translation>Подпись не соответствует данным.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="339"/>
        <source>The signature is valid, but the signer&apos;s key is not fully trusted.</source>
        <translation>Подпись действительна, но ключ подписанта не пользуется полным доверием.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="344"/>
        <source>The signature is valid, but the signer&apos;s key has issues.</source>
        <translation>Подпись действительна, но с ключом подписанта есть проблемы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="348"/>
        <source>The signing key has been revoked.</source>
        <translation>Ключ подписи отозван.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="351"/>
        <source>The signature has expired.</source>
        <translation>Срок действия подписи истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="354"/>
        <source>The signing key has expired.</source>
        <translation>Срок действия ключа подписи истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="357"/>
        <source>The signing key is not authorized to make signatures.</source>
        <translation>Ключ подписи не имеет права создавать подписи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="361"/>
        <source>The signature could not be verified because of an error.</source>
        <translation>Не удалось проверить подпись из-за ошибки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="368"/>
        <source>The signature has an unrecognized status.</source>
        <translation>У подписи нераспознанный статус.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="406"/>
        <source>The operation was cancelled.</source>
        <translation>Операция была отменена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="412"/>
        <source>The active engine does not support this operation.</source>
        <translation>Активный движок не поддерживает эту операцию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="416"/>
        <source>The active engine (%1) does not support this operation.</source>
        <translation>Активный движок (%1) не поддерживает эту операцию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="423"/>
        <source>No result was received for this item before the report was issued.</source>
        <translation>Для этого элемента не был получен результат до формирования отчёта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="429"/>
        <source>The engine finished, but the output was not saved because another item in the batch did not succeed.</source>
        <translation>Движок завершил работу, но выходные данные не сохранены, так как другой элемент пакета завершился неудачно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="438"/>
        <source>The engine finished, but the output could not be applied to the document.</source>
        <translation>Движок завершил работу, но выходные данные не удалось применить к документу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="443"/>
        <source>The output was not saved, and its staged copy could not be removed.</source>
        <translation>Выходные данные не сохранены, а их промежуточную копию не удалось удалить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="448"/>
        <source>The engine finished, but the output could not be saved.</source>
        <translation>Движок завершил работу, но выходные данные не удалось сохранить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="453"/>
        <source>The engine reported an error.</source>
        <translation>Движок сообщил об ошибке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="457"/>
        <source>The engine returned no usable result.</source>
        <translation>Движок не вернул пригодного результата.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="462"/>
        <source>The operation failed without a reported reason.</source>
        <translation>Операция завершилась с ошибкой, но причина не указана.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="469"/>
        <source>The engine reported an error, but the output was kept.</source>
        <translation>Движок сообщил об ошибке, но вывод сохранён.</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="497"/>
        <source>%n key(s) could not be used.</source>
        <translation>
            <numerusform>Не удалось использовать ключей: %n.</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="510"/>
        <source>Review the items below.</source>
        <translation>Просмотрите элементы ниже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="524"/>
        <source>Run the operation again when ready.</source>
        <translation>Запустите операцию снова, когда будете готовы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="527"/>
        <source>Use a key database with an engine that supports this operation.</source>
        <translation>Используйте базу данных ключей с движком, поддерживающим эту операцию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="539"/>
        <source>Check that the destination is writable and has free space, then run the operation again.</source>
        <translation>Проверьте, что в месте назначения разрешена запись и достаточно свободного места, затем повторите операцию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="552"/>
        <source>Import the signer&apos;s public key and verify again.</source>
        <translation>Импортируйте открытый ключ подписанта и повторите проверку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="564"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1765"/>
        <source>Outcome</source>
        <translation>Итог</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="592"/>
        <source>Engine result</source>
        <translation>Результат движка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="598"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1396"/>
        <source>Reason</source>
        <translation>Причина</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="615"/>
        <source>Output</source>
        <translation>Выходные файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1212"/>
        <source>Output error</source>
        <translation>Ошибка вывода</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1760"/>
        <source>Operation</source>
        <translation>Операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1770"/>
        <source>Items</source>
        <translation>Элементы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1776"/>
        <source>Engine</source>
        <translation>Движок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1785"/>
        <source>Reference</source>
        <translation>Ссылка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1791"/>
        <source>Issued</source>
        <translation>Выдан</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1798"/>
        <source>Report hash</source>
        <translation>Хеш отчёта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1829"/>
        <source>Engine report</source>
        <translation>Отчёт движка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="604"/>
        <source>Message</source>
        <translation>Сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="312"/>
        <source>code %1</source>
        <translation>код %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="620"/>
        <source>Suggestion</source>
        <translation>Рекомендация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="626"/>
        <source>Input SHA-256</source>
        <translation>SHA-256 входных данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="663"/>
        <source>Details</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="697"/>
        <source>Some staged output could not be removed; its location is listed below.</source>
        <translation>Часть промежуточных выходных данных не удалось удалить; их расположение указано ниже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="721"/>
        <source>All output files were written.</source>
        <translation>Все выходные файлы записаны.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="724"/>
        <source>No output files were written.</source>
        <translation>Ни один выходной файл не был записан.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="733"/>
        <source>The current document was not changed.</source>
        <translation>Текущий документ не изменён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="777"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="809"/>
        <source>%1 of %2 items completed.</source>
        <translation>%1 из %2 элементов выполнено.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="847"/>
        <source>Current document</source>
        <translation>Текущий документ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="970"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="997"/>
        <source>Succeeded</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="973"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1000"/>
        <source>Completed with warnings</source>
        <translation>Завершено с предупреждениями</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="976"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1015"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="979"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1003"/>
        <source>Cancelled</source>
        <translation>Отменено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="982"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1012"/>
        <source>Not supported</source>
        <translation>Не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="985"/>
        <source>Not reported</source>
        <translation>Не сообщено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="989"/>
        <source>Not saved</source>
        <translation>Не сохранено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1006"/>
        <source>Partially completed</source>
        <translation>Частично завершено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1009"/>
        <source>Incomplete</source>
        <translation>Не завершено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1018"/>
        <source>Partially failed</source>
        <translation>Частично не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1031"/>
        <source>%1 completed (%2 with warnings)</source>
        <translation>Завершено: %1 (%2 с предупреждениями)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1036"/>
        <source>%1 completed</source>
        <translation>Завершено: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1041"/>
        <source>%1 not saved</source>
        <translation>Не сохранено: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1046"/>
        <source>%1 failed</source>
        <translation>Не удалось: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1051"/>
        <source>%1 cancelled</source>
        <translation>Отменено: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1056"/>
        <source>%1 not supported</source>
        <translation>%1 не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1061"/>
        <source>%1 not reported</source>
        <translation>%1 не сообщено</translation>
    </message>
    <message>
        <source>Engine error (%1)</source>
        <translation type="vanished">Ошибка движка (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1183"/>
        <source>Engine error</source>
        <translation>Ошибка движка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1188"/>
        <source>Application error</source>
        <translation>Ошибка приложения</translation>
    </message>
    <message>
        <source>Engine detail (%1)</source>
        <translation type="vanished">Подробности движка (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1195"/>
        <source>Engine detail</source>
        <translation>Подробности движка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1205"/>
        <source>System: %1</source>
        <translation>Система: %1</translation>
    </message>
    <message>
        <source>Output error (%1)</source>
        <translation type="vanished">Ошибка вывода (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1307"/>
        <source>The operation failed. The message below is all that was reported.</source>
        <translation>Операция не удалась. Ниже приведено всё, что было сообщено.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1311"/>
        <source>The operation completed with warnings. The message below is all that was reported.</source>
        <translation>Операция завершилась с предупреждениями. Ниже приведено всё, что было сообщено.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InfoBoardWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="75"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="76"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="77"/>
        <source>Magnify</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="78"/>
        <source>Clear</source>
        <translation>Очистить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="80"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="46"/>
        <source>Copy status text</source>
        <translation>Скопировать текст статуса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="81"/>
        <source>Save status text to file</source>
        <translation>Сохранить текст статуса в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="82"/>
        <source>Magnify the generated document</source>
        <translation>Увеличить масштаб сгенерированного документа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="83"/>
        <source>Clear status panel</source>
        <translation>Очистить панель состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="130"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="565"/>
        <source>Operation status will appear here.</source>
        <translation>Статус операции появится здесь.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="197"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="198"/>
        <source>Details</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="199"/>
        <source>Show the summary report</source>
        <translation>Показать сводный отчёт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="200"/>
        <source>Show the raw status text</source>
        <translation>Показать исходный текст статуса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="669"/>
        <source>The operation completed successfully.</source>
        <translation>Операция завершена успешно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="672"/>
        <source>%1 completed successfully.</source>
        <translation>%1 завершена успешно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1754"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="349"/>
        <source>SECURITY OPERATION REPORT</source>
        <translation>ОТЧЁТ ОБ ОПЕРАЦИИ БЕЗОПАСНОСТИ</translation>
    </message>
    <message>
        <source>Completed with warnings — please review the details.</source>
        <translation type="vanished">Завершено с предупреждениями — пожалуйста, ознакомьтесь с деталями.</translation>
    </message>
    <message>
        <source>%1 completed with warnings — please review the details.</source>
        <translation type="vanished">%1 завершено с предупреждениями — пожалуйста, ознакомьтесь с деталями.</translation>
    </message>
    <message>
        <source>The operation failed. See the details for more information.</source>
        <translation type="vanished">Операция не выполнена. Подробности см. в деталях.</translation>
    </message>
    <message>
        <source>%1 failed. See the details for more information.</source>
        <translation type="vanished">%1 не выполнена. Подробности см. в деталях.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="306"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="308"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="310"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="1022"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="313"/>
        <source>Information</source>
        <translation>Сведения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="322"/>
        <source>The operation finished and everything checked out.</source>
        <translation>Операция завершена, всё проверено.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="324"/>
        <source>The operation finished, but something needs your attention. Read the details below.</source>
        <translation>Операция завершена, но требуется ваше внимание. Подробности см. ниже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="328"/>
        <source>The operation failed, or the result cannot be trusted. Read the details below.</source>
        <translation>Операция не выполнена, или результат не заслуживает доверия. Подробности см. ниже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="333"/>
        <source>Nothing has been checked yet. The light turns green, orange or red once an operation finishes.</source>
        <translation>Ещё ничего не проверено. Индикатор станет зелёным, оранжевым или красным после завершения операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="351"/>
        <source>Green</source>
        <translation>Зелёный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="351"/>
        <source>Everything went well</source>
        <translation>Всё прошло хорошо</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="352"/>
        <source>Orange</source>
        <translation>Оранжевый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="353"/>
        <source>Done, but check the details</source>
        <translation>Выполнено, но проверьте детали</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="354"/>
        <source>Red</source>
        <translation>Красный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="355"/>
        <source>Failed, or the result cannot be trusted</source>
        <translation>Не выполнено, или результат не заслуживает доверия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="356"/>
        <source>Grey</source>
        <translation>Серый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="357"/>
        <source>Idle, nothing checked yet</source>
        <translation>Ожидание, ничего не проверено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidget.cpp" line="390"/>
        <source>[%1] No details available.</source>
        <translation>[%1] Сведения отсутствуют.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="248"/>
        <source>DETAILS</source>
        <translation>Подробности</translation>
    </message>
    <message>
        <source>GPGFRONTEND SECURITY REPORT</source>
        <translation type="vanished">ОТЧЁТ ПО БЕЗОПАСНОСТИ GPGFRONTEND</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetDocument.cpp" line="194"/>
        <source>No operation yet.
Results will appear here as a summary document.</source>
        <translation>Операций пока нет.
Результаты появятся здесь в виде сводного документа.</translation>
    </message>
    <message>
        <source>OPERATION</source>
        <translation type="vanished">ОПЕРАЦИЯ</translation>
    </message>
    <message>
        <source>STATUS</source>
        <translation type="vanished">СТАТУС</translation>
    </message>
    <message>
        <source>ENGINE</source>
        <translation type="vanished">ДВИЖОК</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="44"/>
        <source>Copied</source>
        <translation>Скопировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="120"/>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="153"/>
        <source>Unable to Save</source>
        <translation>Не удалось сохранить</translation>
    </message>
    <message>
        <source>The content could not be captured.</source>
        <translation type="vanished">Не удалось скопировать содержимое.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="92"/>
        <source>GpgFrontend Security Report</source>
        <translation>Отчёт по безопасности GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="93"/>
        <source>Cryptographic operation result report</source>
        <translation>Отчёт о результатах криптографической операции</translation>
    </message>
    <message>
        <source>Hash: </source>
        <translation type="vanished">Хеш:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="121"/>
        <source>The image could not be saved.</source>
        <translation>Не удалось сохранить изображение.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="132"/>
        <source>Export Certificate</source>
        <translation>Экспортировать сертификат</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="133"/>
        <source>PNG Image (*.png);;All Files (*)</source>
        <translation>Изображения PNG (*.png);;Все файлы (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="146"/>
        <source>Save Status Panel Content</source>
        <translation>Сохранить содержимое панели состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="147"/>
        <source>Text Files (*.txt);;All Files (*)</source>
        <translation>Текстовые файлы (*.txt);;Все файлы (*)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetExport.cpp" line="154"/>
        <source>The file could not be saved. Please check the path and permissions.</source>
        <translation>Не удалось сохранить файл. Проверьте путь и права доступа.</translation>
    </message>
    <message>
        <source>File Hash Information</source>
        <translation type="vanished">Информация о хеше файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="208"/>
        <source>Issued:  %1</source>
        <translation>Выдан:  %1</translation>
    </message>
    <message>
        <source>Hash: %1</source>
        <translation type="vanished">Хеш: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="162"/>
        <source>Report status: %1. This seal is GpgFrontend&apos;s report branding; it is not a signature.</source>
        <translation>Статус отчёта: %1. Эта печать — фирменное оформление отчёта GpgFrontend; это не подпись.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="216"/>
        <source>Report hash: %1</source>
        <translation>Хеш отчёта: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="225"/>
        <source>Export certificate as PNG image</source>
        <translation>Экспортировать сертификат в формате PNG</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="277"/>
        <source>… %n more card(s) for this item (%1)</source>
        <translation>
            <numerusform>… ещё %n карточек для этого элемента (%1)</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="279"/>
        <source>Show</source>
        <translation>Показать</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="293"/>
        <source>… %n more item(s): %1</source>
        <translation>
            <numerusform>… ещё %n шт.: %1</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/InfoBoardWidgetResults.cpp" line="295"/>
        <source>Show all</source>
        <translation>Показать все</translation>
    </message>
    <message>
        <source>Operation: %1</source>
        <translation type="vanished">Операция: %1</translation>
    </message>
    <message>
        <source>Status:    %1</source>
        <translation type="vanished">Статус:    %1</translation>
    </message>
    <message>
        <source>SHA-256:   %1</source>
        <translation type="vanished">SHA-256:   %1</translation>
    </message>
    <message>
        <source>Decrypt</source>
        <translation type="vanished">Расшифровать</translation>
    </message>
    <message>
        <source>RECIPIENT</source>
        <translation type="vanished">ПОЛУЧАТЕЛЬ</translation>
    </message>
    <message>
        <source>Sign</source>
        <translation type="vanished">Подписать</translation>
    </message>
    <message>
        <source>Verify</source>
        <translation type="vanished">Проверить</translation>
    </message>
    <message>
        <source>SIGNER</source>
        <translation type="vanished">ПОДПИСАНТ</translation>
    </message>
    <message>
        <source>Engine:    %1</source>
        <translation type="vanished">Движок:    %1</translation>
    </message>
    <message>
        <source>Details:   %1</source>
        <translation type="vanished">Детали:    %1</translation>
    </message>
    <message>
        <source>OK</source>
        <translation type="vanished">ОК</translation>
    </message>
    <message>
        <source>Failed</source>
        <translation type="vanished">Не удалось</translation>
    </message>
    <message>
        <source>… and %1 more</source>
        <translation type="vanished">… и ещё %1</translation>
    </message>
    <message>
        <source>  SHA-256: %1</source>
        <translation type="vanished">SHA-256: %1</translation>
    </message>
    <message>
        <source>  Signer: %1</source>
        <translation type="vanished">Подписант: %1</translation>
    </message>
    <message>
        <source>  Key ID: %1</source>
        <translation type="vanished">Идентификатор ключа: %1</translation>
    </message>
    <message>
        <source>  Signed: %1</source>
        <translation type="vanished">Подписано: %1</translation>
    </message>
    <message>
        <source>  Invalid signer: %1 — %2</source>
        <translation type="vanished">Недействительный подписант: %1 — %2</translation>
    </message>
    <message>
        <source>Encrypt</source>
        <translation type="vanished">Зашифровать</translation>
    </message>
    <message>
        <source>  Recipient: %1</source>
        <translation type="vanished">Получатель: %1</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InfoTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="280"/>
        <source>Developer</source>
        <translation>Разработчик</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="258"/>
        <source>A user-friendly OpenPGP tool for encryption, signing, and key management.</source>
        <translation>Удобный инструмент OpenPGP для шифрования, подписи и управления ключами.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="276"/>
        <source>Developed and maintained by Saturneric.</source>
        <translation>Разработано и поддерживается Saturneric.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="277"/>
        <source>Report an issue on GitHub</source>
        <translation>Сообщить о проблеме на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="277"/>
        <source>About and contact information</source>
        <translation>О программе и контакты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="286"/>
        <source>Website:</source>
        <translation>Веб-сайт:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="289"/>
        <source>Documentation:</source>
        <translation>Документация:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="290"/>
        <source>User guides and overview</source>
        <translation>Руководства пользователя и обзор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="292"/>
        <source>Source code:</source>
        <translation>Исходный код:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="295"/>
        <source>Release notes:</source>
        <translation>Примечания к выпуску:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="296"/>
        <source>Changelog and downloads</source>
        <translation>Список изменений и загрузки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="300"/>
        <source>Resources</source>
        <translation>Ресурсы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="307"/>
        <source>GpgFrontend is free software, licensed under &lt;a href=&quot;https://www.gnu.org/licenses/gpl-3.0.html&quot;&gt;GPL-3.0-or-later&lt;/a&gt;.</source>
        <translation>GpgFrontend — это свободное программное обеспечение, распространяемое под лицензией &lt;a href=&quot;https://www.gnu.org/licenses/gpl-3.0.html&quot;&gt;GPL-3.0-or-later&lt;/a&gt;.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::InstantMessagingTab</name>
    <message>
        <source>Message Book Phrase</source>
        <translation type="vanished">Фраза из книги сообщений</translation>
    </message>
    <message>
        <source>A long secret you share with one friend. It makes your messages look like random text, so nobody can tell they are PGP at all. You and your friend must use exactly the same phrase.</source>
        <translation type="vanished">Длинный секрет, которым вы делитесь с одним другом. Он делает ваши сообщения похожими на случайный текст, так что никто не сможет понять, что это PGP. Вы и ваш друг должны использовать одну и ту же фразу.</translation>
    </message>
    <message>
        <source>No phrase set. Messages use the built-in default book.</source>
        <translation type="vanished">Фраза не задана. Сообщения используют встроенную книгу по умолчанию.</translation>
    </message>
    <message>
        <source>Generate</source>
        <translation type="vanished">Сгенерировать</translation>
    </message>
    <message>
        <source>Create a new random phrase. Share it with your friend so you both use the same one.</source>
        <translation type="vanished">Создать новую случайную фразу. Поделитесь ею с другом, чтобы вы оба использовали одну и ту же.</translation>
    </message>
    <message>
        <source>Show</source>
        <translation type="vanished">Показать</translation>
    </message>
    <message>
        <source>Show or hide the phrase.</source>
        <translation type="vanished">Показать или скрыть фразу.</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation type="vanished">Копировать</translation>
    </message>
    <message>
        <source>Copy the phrase to the clipboard.</source>
        <translation type="vanished">Скопировать фразу в буфер обмена.</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation type="vanished">Вставить</translation>
    </message>
    <message>
        <source>Replace the phrase with the one on the clipboard.</source>
        <translation type="vanished">Заменить фразу на ту, что в буфере обмена.</translation>
    </message>
    <message>
        <source>Clear</source>
        <translation type="vanished">Очистить</translation>
    </message>
    <message>
        <source>Remove the phrase and fall back to the default book.</source>
        <translation type="vanished">Удалить фразу и вернуться к книге по умолчанию.</translation>
    </message>
    <message>
        <source>Book Fingerprint</source>
        <translation type="vanished">Отпечаток книги</translation>
    </message>
    <message>
        <source>A short code made from your phrase. Read it out with your friend to be sure you both have the same one. Unlike the phrase, this code is safe to say out loud.</source>
        <translation type="vanished">Короткий код, созданный из вашей фразы. Прочитайте его вслух вместе с другом, чтобы убедиться, что у вас обоих одна и та же фраза. В отличие от фразы, этот код можно произносить вслух.</translation>
    </message>
    <message>
        <source>Copy the fingerprint to the clipboard.</source>
        <translation type="vanished">Скопировать отпечаток в буфер обмена.</translation>
    </message>
    <message>
        <source>The phrase is stored in the encrypted cache, never in the settings file. Send it to your friend over a private channel.</source>
        <translation type="vanished">Фраза хранится в зашифрованном кеше, а не в файле настроек. Отправьте её другу по приватному каналу.</translation>
    </message>
    <message>
        <source>Hide</source>
        <translation type="vanished">Скрыть</translation>
    </message>
    <message>
        <source>No phrase set. Using the built-in default.</source>
        <translation type="vanished">Фраза не задана. Используется встроенная по умолчанию.</translation>
    </message>
    <message>
        <source>Phrase set. %1 characters.</source>
        <translation type="vanished">Фраза задана. %1 символов.</translation>
    </message>
    <message>
        <source>Calculating…</source>
        <translation type="vanished">Вычисление…</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::IntroPage</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="314"/>
        <source>Welcome to GpgFrontend</source>
        <translation>Добро пожаловать в GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="315"/>
        <source>Let&apos;s get you started in just a moment.</source>
        <translation>Совсем скоро мы поможем вам начать работу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="318"/>
        <source>A simple, privacy-focused OpenPGP tool for text, files, and keys.</source>
        <translation>Простой инструмент OpenPGP, ориентированный на конфиденциальность, для текста, файлов и ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="321"/>
        <source>Language:</source>
        <translation>Язык:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="322"/>
        <source>System Default</source>
        <translation>По умолчанию в системе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="325"/>
        <source>&lt;b&gt;GpgFrontend&lt;/b&gt; helps you encrypt, decrypt, sign, and verify messages and files with OpenPGP. This short wizard will point you to the most useful places to start.</source>
        <translation>&lt;b&gt;GpgFrontend&lt;/b&gt; помогает шифровать, расшифровывать, подписывать и проверять сообщения и файлы с помощью OpenPGP. Этот краткий мастер поможет вам начать работу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="344"/>
        <source>You can change update checking, key database, and appearance settings later from the application settings.</source>
        <translation>Вы можете изменить настройки проверки обновлений, базу данных ключей и внешний вид позже в настройках приложения.</translation>
    </message>
    <message>
        <source>You can change language, update checking, key database, and appearance settings later from the application settings.</source>
        <translation type="vanished">Вы можете изменить язык, настройки проверки обновлений, базу данных ключей и внешний вид позже в настройках приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="330"/>
        <source>Star GpgFrontend on GitHub</source>
        <translation>Поставить звезду GpgFrontend на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="331"/>
        <source>GpgFrontend is free and open source. A star helps more people discover it and keeps the project moving forward.</source>
        <translation>GpgFrontend — бесплатное приложение с открытым исходным кодом. Звезда помогает большему количеству людей узнать о нём и способствует развитию проекта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="335"/>
        <source>Open the overview page</source>
        <translation>Открыть страницу обзора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="336"/>
        <source>Get a quick tour of the main features and common workflows.</source>
        <translation>Краткий обзор основных функций и типичных рабочих процессов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="339"/>
        <source>Fundamental concepts</source>
        <translation>Основные понятия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="340"/>
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
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="138"/>
        <source>This key carries no email address.</source>
        <translation>У этого ключа нет адреса электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="175"/>
        <source>Certifying is for vouching for someone else&apos;s key.</source>
        <translation>Сертификация предназначена для подтверждения чужого ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="197"/>
        <source>Tick the box beside at least one key first.</source>
        <translation>Сначала установите флажок рядом хотя бы с одним ключом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="219"/>
        <source>Expiry can only be changed on keys you own.</source>
        <translation>Срок действия можно изменить только для ключей, которыми вы владеете.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyActionState.cpp" line="230"/>
        <source>This keyring holds no private keys.</source>
        <translation>Эта связка ключей не содержит закрытых ключей.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyCategoryEditDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="186"/>
        <source>Edit Category</source>
        <translation>Изменить категорию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="186"/>
        <source>New Category</source>
        <translation>Новая категория</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="191"/>
        <source>Edit Key Category</source>
        <translation>Изменить категорию ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="191"/>
        <source>Create a Key Category</source>
        <translation>Создать категорию ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="198"/>
        <source>Categories sort keys into your own groups in the key list. A key can belong to several categories.</source>
        <translation>Категории группируют ключи в ваши собственные группы в списке ключей. Ключ может принадлежать нескольким категориям.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="222"/>
        <source>e.g. Work Contacts</source>
        <translation>например, Рабочие контакты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="268"/>
        <source>Custom Colour</source>
        <translation>Пользовательский цвет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="270"/>
        <source>Custom Colour...</source>
        <translation>Пользовательский цвет...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="285"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="286"/>
        <source>Colour</source>
        <translation>Цвет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="294"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="294"/>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="295"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="308"/>
        <source>%n selected key(s) will be added to this category.</source>
        <translation>
            <numerusform>%n выбранных ключей будет добавлено в эту категорию.</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="362"/>
        <source>Custom Colour (%1)...</source>
        <translation>Пользовательский цвет (%1)...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="373"/>
        <source>Choose Category Colour</source>
        <translation>Выбор цвета категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryEditDialog.cpp" line="394"/>
        <source>Category Name</source>
        <translation>Имя категории</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyCategoryRules</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="82"/>
        <source>The name is too long.</source>
        <translation>Имя слишком длинное.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="86"/>
        <source>A category with this name already exists.</source>
        <translation>Категория с таким именем уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="101"/>
        <source>Red</source>
        <translation>Красный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="102"/>
        <source>Orange</source>
        <translation>Оранжевый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="104"/>
        <source>Amber</source>
        <translation>Янтарный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="106"/>
        <source>Yellow</source>
        <translation>Жёлтый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="108"/>
        <source>Lime</source>
        <translation>Лаймовый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="110"/>
        <source>Green</source>
        <translation>Зелёный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="112"/>
        <source>Emerald</source>
        <translation>Изумрудный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="114"/>
        <source>Teal</source>
        <translation>Бирюзовый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="116"/>
        <source>Cyan</source>
        <translation>Голубой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="119"/>
        <source>Sky</source>
        <translation>Небесно-голубой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="120"/>
        <source>Blue</source>
        <translation>Синий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="122"/>
        <source>Indigo</source>
        <translation>Индиго</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="124"/>
        <source>Violet</source>
        <translation>Фиолетовый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="126"/>
        <source>Purple</source>
        <translation>Пурпурный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="128"/>
        <source>Fuchsia</source>
        <translation>Фуксия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="130"/>
        <source>Pink</source>
        <translation>Розовый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="132"/>
        <source>Rose</source>
        <translation>Роза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="134"/>
        <source>Brown</source>
        <translation>Коричневый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="136"/>
        <source>Navy</source>
        <translation>Темно-синий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyCategoryRules.cpp" line="138"/>
        <source>Stone</source>
        <translation>Каменный</translation>
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
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="140"/>
        <source>Convert to Relative Path</source>
        <translation>Преобразовать в относительный путь</translation>
    </message>
    <message>
        <source>Key Database Info</source>
        <translation type="vanished">Информация о базе данных ключей</translation>
    </message>
    <message>
        <source>Key Database Path (Automatically Generated)</source>
        <translation type="vanished">Путь к базе данных ключей (автоматически сгенерирован)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="136"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <source>Path</source>
        <translation type="vanished">Путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="138"/>
        <source>Engine</source>
        <translation>Движок</translation>
    </message>
    <message>
        <source>Mode</source>
        <translation type="vanished">Режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="139"/>
        <source>Choose Folder…</source>
        <translation>Выберите папку…</translation>
    </message>
    <message>
        <source>Automatic</source>
        <translation type="vanished">Автоматический</translation>
    </message>
    <message>
        <source>Manual</source>
        <translation type="vanished">Ручной</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="137"/>
        <source>Folder</source>
        <translation>Папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="141"/>
        <source>e.g. Personal Keys</source>
        <translation>например, Личные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="159"/>
        <source>Open Directory</source>
        <translation>Открыть каталог</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="171"/>
        <source>Illegal GnuPG Key Database Path</source>
        <translation>Недопустимый путь к базе данных ключей GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="172"/>
        <source>Target GnuPG Key Database Path is not an exists readable directory.</source>
        <translation>Целевой путь к базе данных ключей GnuPG не является существующим доступным для чтения каталогом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="228"/>
        <source>Add Key Database</source>
        <translation>Добавить базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="229"/>
        <source>New key database in your profile</source>
        <translation>Новая база данных ключей в вашем профиле</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="231"/>
        <source>GpgFrontend picks the folder and keeps it inside your profile, so this key database travels with it. Just choose a name.</source>
        <translation>GpgFrontend выбирает папку и хранит её внутри вашего профиля, поэтому эта база данных ключей перемещается вместе с ним. Просто выберите имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="236"/>
        <source>Rename Key Database</source>
        <translation>Переименовать базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="237"/>
        <source>Rename this key database</source>
        <translation>Переименовать эту базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="238"/>
        <source>The folder it is kept in is named after it, and is renamed too.</source>
        <translation>Папка, в которой она хранится, названа по её имени и тоже переименовывается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="243"/>
        <source>Add Key Database On This Computer</source>
        <translation>Добавить базу данных ключей на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="244"/>
        <source>New key database on this computer</source>
        <translation>Новая база данных ключей на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="246"/>
        <source>Choose a folder yourself. A key database outside your profile belongs to this computer alone, and a profile package never carries it.</source>
        <translation>Выберите папку самостоятельно. База данных ключей вне вашего профиля принадлежит только этому компьютеру, и пакет профиля никогда не включает её.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="252"/>
        <source>Edit Key Database</source>
        <translation>Редактировать базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="253"/>
        <source>Key database on this computer</source>
        <translation>База данных ключей на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="255"/>
        <source>This key database belongs to this computer alone, and a profile package never carries it.</source>
        <translation>Эта база данных ключей принадлежит только этому компьютеру, и пакет профиля никогда не переносит её.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="305"/>
        <source>Choose a name first</source>
        <translation>Сначала выберите имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="306"/>
        <source>No folder chosen yet</source>
        <translation>Папка ещё не выбрана.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="362"/>
        <source>&quot;%1&quot; is reserved for the Built-in Key Database. Turn that one on in its section of the Key Databases settings, or pick another name.</source>
        <translation>«%1» зарезервировано для встроенной базы данных ключей. Включите её в соответствующем разделе настроек баз данных ключей или выберите другое имя.</translation>
    </message>
    <message>
        <source>GpgFrontend picks a safe location for you. Just choose a name.</source>
        <translation type="vanished">GpgFrontend выберет безопасное расположение за вас. Просто укажите имя.</translation>
    </message>
    <message>
        <source>Pick an existing folder yourself. For advanced setups.</source>
        <translation type="vanished">Выберите существующую папку самостоятельно. Для расширенных настроек.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="353"/>
        <source>The key database name cannot be empty.</source>
        <translation>Имя базы данных ключей не может быть пустым.</translation>
    </message>
    <message>
        <source>&quot;%1&quot; is the name of the key database GpgFrontend derives from your OpenPGP engine. Turn that one on with the checkbox in Key Database settings, or pick another name.</source>
        <translation type="vanished">«%1» — это имя базы данных ключей, которое GpgFrontend получает из вашего OpenPGP-движка. Включите её с помощью флажка в настройках базы данных ключей или выберите другое имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="370"/>
        <source>The key database path cannot be empty.</source>
        <translation>Путь к базе данных ключей не может быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyDatabaseEditDialog.cpp" line="381"/>
        <source>A key database with the name &apos;%1&apos; already exists. Please choose a different name.</source>
        <translation>База данных ключей с именем «%1» уже существует. Выберите другое имя.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDatabasesTab</name>
    <message>
        <source>Name</source>
        <translation type="vanished">Имя</translation>
    </message>
    <message>
        <source>Backend Type</source>
        <translation type="vanished">Тип бэкенда</translation>
    </message>
    <message>
        <source>Status</source>
        <translation type="vanished">Статус</translation>
    </message>
    <message>
        <source>Path</source>
        <translation type="vanished">Путь</translation>
    </message>
    <message>
        <source>Real Path</source>
        <translation type="vanished">Реальный путь</translation>
    </message>
    <message>
        <source>Add New Key Database</source>
        <translation type="vanished">Добавить новую базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="409"/>
        <source>Unreadable Key Database Settings</source>
        <translation>Нечитаемые настройки базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="410"/>
        <source>Your saved key database list exists but could not be read. This usually means it was written by another installation, or with a different application key.</source>
        <translation>Ваш сохранённый список баз данных ключей существует, но не может быть прочитан. Обычно это означает, что он был записан другой установкой или с другим ключом приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="414"/>
        <source>Saving now replaces it with the list shown here. The previous list cannot be recovered afterwards.</source>
        <translation>Сохранение сейчас заменит его на список, показанный здесь. Предыдущий список не может быть восстановлен впоследствии.</translation>
    </message>
    <message>
        <source>N/A</source>
        <translation type="vanished">Н/Д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="945"/>
        <source>No Key Database Selected</source>
        <translation>База данных ключей не выбрана</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="946"/>
        <source>Please select a key database to edit.</source>
        <translation>Пожалуйста, выберите базу данных ключей для редактирования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="899"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1020"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1109"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1173"/>
        <source>Duplicate Key Database Paths</source>
        <translation>Дублирующиеся пути к базам данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1021"/>
        <source>The edited key database path duplicates a previously existing one.</source>
        <translation>Указанный путь к базе данных ключей дублирует уже существующий.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1012"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1098"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1165"/>
        <source>Invalid Key Database Paths</source>
        <translation>Недопустимые пути к базам данных ключей</translation>
    </message>
    <message>
        <source>Use this computer&apos;s default key database</source>
        <translation type="vanished">Использовать базу данных ключей по умолчанию этого компьютера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="256"/>
        <source>Add Key Database</source>
        <translation>Добавить базу данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="258"/>
        <source>Add Key Database On This Computer</source>
        <translation>Добавить базу данных ключей на этом компьютере</translation>
    </message>
    <message>
        <source>The OpenPGP engine on this computer does not report a key database, so there is no default one to use.</source>
        <translation type="vanished">Движок OpenPGP на этом компьютере не сообщает о базе данных ключей, поэтому нет базы данных по умолчанию.</translation>
    </message>
    <message>
        <source>Always available on this computer, and always channel 0.</source>
        <translation type="vanished">Всегда доступна на этом компьютере и всегда канал 0.</translation>
    </message>
    <message>
        <source>Always channel 0. Never included in a profile package: whoever opens your profile elsewhere gets their own.</source>
        <translation type="vanished">Всегда канал 0. Никогда не включается в пакет профиля: любой, кто откроет ваш профиль в другом месте, получит свою собственную базу данных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="775"/>
        <source>This folder could not be resolved on this computer.</source>
        <translation>Не удалось найти эту папку на этом компьютере.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="804"/>
        <source>No key databases of your own yet. One you add here is kept inside your profile and travels with it.</source>
        <translation>У вас ещё нет собственных баз данных ключей. Добавленная здесь база данных хранится внутри вашего профиля и перемещается вместе с ним.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="806"/>
        <source>Kept inside your profile, and carried by a profile package.</source>
        <translation>Хранится внутри вашего профиля и переносится пакетом профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="810"/>
        <source>Nothing here yet. A key database you add on this tab stays on this computer: a profile package never carries it, and it is always opened after the ones inside your profile.</source>
        <translation>Здесь пока ничего нет. База данных ключей, добавленная на этой вкладке, остаётся на этом компьютере: пакет профиля никогда не переносит её, и она всегда открывается после баз данных внутри вашего профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="813"/>
        <source>Key databases outside your profile. They belong to this computer alone: a profile package never carries them, and they are always opened after the ones inside your profile.</source>
        <translation>Базы данных ключей вне вашего профиля. Они принадлежат только этому компьютеру: пакет профиля никогда не переносит их, и они всегда открываются после баз данных внутри вашего профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="877"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1063"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1145"/>
        <source>Currently, GpgFrontend supports a maximum of %1 key databases. Please remove an existing database to add a new one.</source>
        <translation>В настоящее время GpgFrontend поддерживает максимум %1 баз данных ключей. Удалите существующую базу данных, чтобы добавить новую.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="900"/>
        <source>A key database on this page already uses this computer&apos;s default key database folder. Remove it first, then turn this on.</source>
        <translation>Одна база данных ключей на этой странице уже использует папку базы данных ключей по умолчанию на этом компьютере. Сначала удалите её, затем включите этот параметр.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="923"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1214"/>
        <source>Key Database Required</source>
        <translation>Требуется база данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="924"/>
        <source>GpgFrontend needs at least one key database. Add another one first, then turn this off.</source>
        <translation>GpgFrontend нуждается как минимум в одной базе данных ключей. Сначала добавьте другую, затем отключите эту.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="990"/>
        <source>Key Database Folder Already Exists</source>
        <translation>Папка базы данных ключей уже существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="991"/>
        <source>A folder named after &apos;%1&apos; is already there. Its contents are not ours to replace, so choose another name.</source>
        <translation>Папка с именем «%1» уже существует. Мы не вправе заменять её содержимое, поэтому выберите другое имя.</translation>
    </message>
    <message>
        <source>Failed to Rename Key Database Folder</source>
        <translation type="vanished">Не удалось переименовать папку базы данных ключей</translation>
    </message>
    <message>
        <source>GpgFrontend could not rename the folder this key database is kept in. It may still be in use; try again after restarting GpgFrontend.</source>
        <translation type="vanished">GpgFrontend не удалось переименовать папку, в которой хранится эта база данных ключей. Возможно, она всё ещё используется; повторите попытку после перезапуска GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1013"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1099"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1166"/>
        <source>The edited key database path is not a valid path that GpgFrontend can use</source>
        <translation>Указанный путь к базе данных ключей не является допустимым путем, который может использовать GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="876"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1062"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1144"/>
        <source>Maximum Key Database Limit Reached</source>
        <translation>Достигнут лимит баз данных ключей</translation>
    </message>
    <message>
        <source>Currently, GpgFrontend supports a maximum of 8 key databases. Please remove an existing database to add a new one.</source>
        <translation type="vanished">В настоящее время GpgFrontend поддерживает максимум 8 баз данных ключей. Пожалуйста, удалите существующую базу данных, чтобы добавить новую.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1110"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1174"/>
        <source>The newly added key database path duplicates a previously existing one.</source>
        <translation>Добавленный путь к базе данных ключей дублирует уже существующий.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1078"/>
        <source>Invalid Key Database Path</source>
        <translation>Недопустимый путь к базе данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="191"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="637"/>
        <source>Used when an operation does not explicitly select a key database.</source>
        <translation>Используется, когда операция не указывает базу данных ключей явно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="193"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="250"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="261"/>
        <source>Use as Default</source>
        <translation>Использовать по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="195"/>
        <source>Copy Folder Path</source>
        <translation>Копировать путь к папке</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="215"/>
        <source>Built-in Key Database</source>
        <translation>Встроенная база данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="217"/>
        <source>Key engine:</source>
        <translation>Движок ключей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="223"/>
        <source>Key engine for the built-in database</source>
        <translation>Движок ключей для встроенной базы данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="227"/>
        <source>Applies only to the built-in database. Other databases always open with their own engine.</source>
        <translation>Применяется только к встроенной базе данных. Другие базы данных всегда открываются со своим движком.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="231"/>
        <source>Location:</source>
        <translation>Расположение:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="236"/>
        <source>Built-in key database location</source>
        <translation>Расположение встроенной базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="238"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="239"/>
        <source>Copy the full path</source>
        <translation>Копировать полный путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="241"/>
        <source>Copy the built-in key database location</source>
        <translation>Копировать расположение встроенной базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="247"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="656"/>
        <source>Default</source>
        <translation>По умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="253"/>
        <source>Use the built-in key database as default</source>
        <translation>Использовать встроенную базу данных ключей по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="263"/>
        <source>Use the selected key database as default</source>
        <translation>Использовать выбранную базу данных ключей по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="267"/>
        <source>Smart cards stay with the previous default database until GpgFrontend restarts. To move them sooner, apply this change, then use &quot;Restart All Gpg-Agents&quot; in the Smart Card Controller.</source>
        <translation>Смарт-карты остаются привязанными к предыдущей базе данных по умолчанию до перезапуска GpgFrontend. Чтобы перенести их раньше, примените это изменение, а затем используйте «Перезапустить все Gpg-Agents» в контроллере смарт-карт.</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="531"/>
        <source>%1: %n operation(s)</source>
        <translation>
            <numerusform>%1: %n операций</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="573"/>
        <source>The OpenPGP engine on this computer does not report a key database, so there is no built-in one to use.</source>
        <translation>OpenPGP-движок на этом компьютере не сообщает о базе данных ключей, поэтому встроенной базы данных для использования нет.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="585"/>
        <source>Always available on this computer, and always listed first.</source>
        <translation>Всегда доступна на этом компьютере и всегда отображается первой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="586"/>
        <source>Always listed first. Never included in a profile package: whoever opens your profile elsewhere gets their own.</source>
        <translation>Всегда отображается первым. Никогда не включается в пакет профиля: у того, кто откроет ваш профиль в другом месте, будет собственная.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="600"/>
        <source>No engine in this build can open it.</source>
        <translation>Ни один движок в этой сборке не может её открыть.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1079"/>
        <source>The specified key database path points to an existing file. Please specify a path that does not exist or points to a directory.</source>
        <translation>Указанный путь к базе данных ключей указывает на существующий файл. Пожалуйста, укажите путь, который не существует или указывает на каталог.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1088"/>
        <source>Failed to Create Key Database Directory</source>
        <translation>Не удалось создать каталог базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1089"/>
        <source>GpgFrontend failed to create a directory at the specified key database path. Please check the path and your permissions.</source>
        <translation>GpgFrontend не удалось создать каталог по указанному пути к базе данных ключей. Пожалуйста, проверьте путь и ваши права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1215"/>
        <source>GpgFrontend needs at least one key database, so the last one cannot be removed.</source>
        <translation>GpgFrontend нуждается как минимум в одной базе данных ключей, поэтому последнюю удалить нельзя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1221"/>
        <source>Confirm Deletion</source>
        <translation>Подтверждение удаления</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="1222"/>
        <source>Are you sure you want to remove the selected key database from the list?</source>
        <translation>Вы уверены, что хотите удалить выбранную базу данных ключей из списка?</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyDetailsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="67"/>
        <source>KeyPair</source>
        <translation>Ключевая пара</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="80"/>
        <source>UIDs</source>
        <translation>Идентификаторы пользователя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="86"/>
        <source>Photo IDs</source>
        <translation>Фото-идентификаторы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="91"/>
        <source>Keychain</source>
        <translation>Связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="94"/>
        <source>Operations</source>
        <translation>Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="134"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyDetailsDialog.cpp" line="139"/>
        <source>Key Details</source>
        <translation>Сведения о ключе</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGenerateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1396"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1491"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1497"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1598"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1767"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1805"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="167"/>
        <source>Custom</source>
        <translation>Пользовательский</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="172"/>
        <source>3 Months</source>
        <translation>3 месяца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="175"/>
        <source>6 Months</source>
        <translation>6 месяцев</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="178"/>
        <source>1 Year</source>
        <translation>1 год</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="164"/>
        <source>2 Years</source>
        <translation>2 года</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="182"/>
        <source>5 Years</source>
        <translation>5 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="185"/>
        <source>10 Years</source>
        <translation>10 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="537"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="548"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.h" line="188"/>
        <source>Non Expired</source>
        <translation>Действующий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="492"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="884"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1502"/>
        <source>Primary Key Only</source>
        <translation>Только первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="493"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="947"/>
        <source>Primary Key With Subkey</source>
        <translation>Первичный ключ с подключом</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="499"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="508"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="500"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="501"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="507"/>
        <source>Key Database</source>
        <translation>База данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="521"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="539"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="509"/>
        <source>Combination</source>
        <translation>Комбинация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="496"/>
        <source>Profile</source>
        <translation>Профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="510"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="522"/>
        <source>Validity Period</source>
        <translation>Срок действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1753"/>
        <source>Save Profile</source>
        <translation>Сохранить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="514"/>
        <source>Save current configuration as a new profile</source>
        <translation>Сохранить текущую конфигурацию как новый профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1824"/>
        <source>Delete Profile</source>
        <translation>Удалить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="516"/>
        <source>Delete current selected profile</source>
        <translation>Удалить текущий выбранный профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="497"/>
        <source>Basic</source>
        <translation>Основные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="512"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="515"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="573"/>
        <source>%1 Engine Version: %2</source>
        <translation>Версия движка %1: %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="728"/>
        <source>Name must not be empty.</source>
        <translation>Имя не должно быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1851"/>
        <source>Reset To Default</source>
        <translation>Сбросить по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="519"/>
        <source>Reset profile list to default configuration</source>
        <translation>Сбросить список профилей к настройкам по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="505"/>
        <source>Fill in a random anonymous identity (for throwaway or test keys)</source>
        <translation>Заполнить случайный анонимный идентификатор (для одноразовых или тестовых ключей)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="517"/>
        <source>Reset</source>
        <translation>Сбросить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="523"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="541"/>
        <source>Key Length</source>
        <translation>Длина ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="524"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="543"/>
        <source>Second Algorithm</source>
        <translation>Второй алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="525"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="544"/>
        <source>Second Key Length</source>
        <translation>Длина второго ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="526"/>
        <source>Key Format</source>
        <translation>Формат ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="527"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="542"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="531"/>
        <source>v4 (Compatible)</source>
        <translation>v4 (Совместимый)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="532"/>
        <source>v6 (Modern)</source>
        <translation>v6 (Современный)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="533"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="545"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="534"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="546"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="535"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="547"/>
        <source>Authentication</source>
        <translation>Аутентификация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="536"/>
        <source>No Passphrase</source>
        <translation>Без парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="540"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="551"/>
        <source>Easy Mode</source>
        <translation>Простой режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="552"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="553"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="556"/>
        <source>Generate</source>
        <translation>Сгенерировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="578"/>
        <source>If subkey is specified, it will be generated together with the primary key. Therefore, you may need to enter the passphrase additionally for the subkey generation.</source>
        <translation>Если указан подключ, он будет сгенерирован вместе с первичным ключом. Поэтому может потребоваться дополнительный ввод парольной фразы для генерации подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="584"/>
        <source>Supported Primary Key Algorithms: </source>
        <translation>Поддерживаемые алгоритмы первичного ключа: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="594"/>
        <source>Supported Subkey Algorithms: </source>
        <translation>Поддерживаемые алгоритмы подключа: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="604"/>
        <source>Please select a key algorithm and configure the parameters as needed.</source>
        <translation>Выберите алгоритм ключа и настройте параметры.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="614"/>
        <source>Generate Key</source>
        <translation>Сгенерировать ключ</translation>
    </message>
    <message>
        <source>Name must contain at least five characters.</source>
        <translation type="vanished">Имя должно содержать не менее пяти символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="734"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Имя и комментарий не должны содержать символы &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; или управляющие символы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="740"/>
        <source>Please give a valid email address.</source>
        <translation>Укажите корректный адрес электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="745"/>
        <source>Please give a valid primary key algorithm.</source>
        <translation>Укажите допустимый алгоритм первичного ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="754"/>
        <source>Time to primary key expiration must not be less than 120 seconds.</source>
        <translation>Время до истечения первичного ключа должно быть не менее 120 секунд.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="761"/>
        <source>Please give a valid subkey algorithm.</source>
        <translation>Укажите допустимый алгоритм подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="769"/>
        <source>Time to subkey expiration must not be less than 120 seconds.</source>
        <translation>Время до истечения подключа должно быть не менее 120 секунд.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="799"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="800"/>
        <source>Key generation completed successfully.</source>
        <translation>Генерация ключа успешно завершена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1527"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1528"/>
        <source>Unknown error occurred</source>
        <translation>Произошла неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1544"/>
        <source>Generating</source>
        <translation>Генерация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1675"/>
        <source>ECC</source>
        <translation>ECC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1680"/>
        <source>Post-Quantum</source>
        <translation>Постквантовый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1754"/>
        <source>Please enter profile name:</source>
        <translation>Введите имя профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1762"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1768"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1776"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1790"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1806"/>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1818"/>
        <source>Notice</source>
        <translation>Уведомление</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1763"/>
        <source>Profile was not saved: Name cannot be empty.</source>
        <translation>Профиль не сохранён: имя не может быть пустым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1769"/>
        <source>The profile name &apos;Custom&apos; is reserved. Please choose another name.</source>
        <translation>Имя профиля «Custom» зарезервировано. Выберите другое имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1777"/>
        <source>Profile was not saved: Name cannot be longer than %1 characters.</source>
        <translation>Профиль не сохранён: имя не может быть длиннее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1791"/>
        <source>Profile was not saved: Name already exists.</source>
        <translation>Профиль не сохранён: имя уже существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1807"/>
        <source>The &apos;Custom&apos; profile cannot be deleted.</source>
        <translation>Профиль «Custom» нельзя удалить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1819"/>
        <source>Selected profile does not exist.</source>
        <translation>Выбранный профиль не существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1825"/>
        <source>Are you sure you want to delete the profile &apos;%1&apos;?</source>
        <translation>Вы уверены, что хотите удалить профиль «%1»?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="1852"/>
        <source>Are you sure you want to reset the easy profile configuration to default? This action cannot be undone.</source>
        <translation>Вы уверены, что хотите сбросить конфигурацию простого профиля по умолчанию? Это действие нельзя отменить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/KeyGenerateDialog.cpp" line="2019"/>
        <source>Post-quantum algorithms require the v6 key format.</source>
        <translation>Постквантовые алгоритмы требуют формат ключа v6.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupCreationDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="103"/>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <source>A Key Group is a collection of keys. It allows you to encrypt data for multiple recipients at once by grouping their public keys together.</source>
        <translation type="vanished">Группа ключей — это набор ключей. Она позволяет шифровать данные для нескольких получателей одновременно, объединяя их открытые ключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="58"/>
        <source>Engineering Team</source>
        <translation>Инженерная команда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="61"/>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="64"/>
        <source>Optional</source>
        <translation>Необязательно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="66"/>
        <source>Create a Key Group</source>
        <translation>Создать группу ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="72"/>
        <source>Encrypting to a key group encrypts to every key it contains.</source>
        <translation>Шифрование для группы ключей шифрует для каждого ключа, входящего в её состав.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="92"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="94"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="96"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="104"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupCreationDialog.cpp" line="52"/>
        <source>New Key Group</source>
        <translation>Новая группа ключей</translation>
    </message>
    <message>
        <source>Name must contain at least five characters.</source>
        <translation type="vanished">Имя должно содержать не менее пяти символов.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupEditDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="44"/>
        <source>Edit Key Group</source>
        <translation>Редактировать группу ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="52"/>
        <source>Edit Key Group Details</source>
        <translation>Редактирование сведений группы ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="58"/>
        <source>These details only name the group. Changing them does not affect which keys belong to it.</source>
        <translation>Эти сведения только задают имя группы. Их изменение не влияет на то, какие ключи в неё входят.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="75"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="77"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="79"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="86"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupEditDialog.cpp" line="87"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupManageDialog</name>
    <message>
        <source>Key Group</source>
        <translation type="vanished">Группа ключей</translation>
    </message>
    <message>
        <source>Default</source>
        <translation type="vanished">По умолчанию</translation>
    </message>
    <message>
        <source>Key(s) in Key Group: </source>
        <translation type="vanished">Ключ(и) в группе ключей: </translation>
    </message>
    <message>
        <source>Key(s) in Key Database: </source>
        <translation type="vanished">Ключ(и) в базе данных ключей: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="100"/>
        <source>Key Group Management</source>
        <translation>Управление группами ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="130"/>
        <source>Encrypting to this group encrypts to every key it contains, including the members of any group nested inside it.</source>
        <translation>Шифрование для этой группы применяется ко всем ключам в ней, включая участников вложенных групп.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="140"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="179"/>
        <source>Edit…</source>
        <translation>Изменить…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="181"/>
        <source>Change the name, email and comment of this key group.</source>
        <translation>Изменить имя, адрес электронной почты и комментарий этой группы ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="187"/>
        <source>Delete Group…</source>
        <translation>Удалить группу…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="189"/>
        <source>Delete this key group. The keys in it are not touched.</source>
        <translation>Удалить эту группу ключей. Ключи в ней не затрагиваются.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="213"/>
        <source>Filter members</source>
        <translation>Фильтр участников</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="241"/>
        <source>This group has no members yet.

Check keys on the right and press Add.</source>
        <translation>В этой группе ещё нет участников.

Отметьте ключи справа и нажмите «Добавить».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="269"/>
        <source>Members</source>
        <translation>Участники</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="284"/>
        <source>Add</source>
        <translation>Добавить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="287"/>
        <source>Add the keys checked on the right to this group.</source>
        <translation>Добавить отмеченные справа ключи в эту группу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="292"/>
        <source>Remove</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="295"/>
        <source>Remove the members checked on the left from this group.</source>
        <translation>Удалить отмеченные слева ключи из этой группы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="324"/>
        <source>Available</source>
        <translation>Доступные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="348"/>
        <source>Every key that can be added is already in this group.</source>
        <translation>Все ключи, которые можно добавить, уже в этой группе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="356"/>
        <source>Available Keys</source>
        <translation>Доступные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="423"/>
        <source>created %1</source>
        <translation>создан %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="477"/>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="498"/>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="626"/>
        <source>Some Keys Failed</source>
        <translation>Не удалось обработать некоторые ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="478"/>
        <source>Some keys could not be added to the group:
%1</source>
        <translation>Некоторые ключи не удалось добавить в группу:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="499"/>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="627"/>
        <source>Some keys could not be removed from the group:
%1</source>
        <translation>Некоторые ключи не удалось удалить из группы:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="534"/>
        <source>Update Failed</source>
        <translation>Не удалось обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="535"/>
        <source>This key group could not be updated.</source>
        <translation>Не удалось обновить эту группу ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="560"/>
        <source>Delete Key Group</source>
        <translation>Удалить группу ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="573"/>
        <source>Delete Failed</source>
        <translation>Не удалось удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="574"/>
        <source>This key group could not be deleted.</source>
        <translation>Не удалось удалить эту группу ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="618"/>
        <source>Remove from Group</source>
        <translation>Удалить из группы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="633"/>
        <source>Manage Group…</source>
        <translation>Управление группой…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="638"/>
        <source>Key Details…</source>
        <translation>Сведения о ключе…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="661"/>
        <source>This Key Group contains some invalid keys:

%1

These keys are no longer available. Do you want to remove them from the group?</source>
        <translation>Эта группа ключей содержит недопустимые ключи:

%1

Эти ключи больше недоступны. Хотите удалить их из группы?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupManageDialog.cpp" line="667"/>
        <source>Invalid Keys in Group</source>
        <translation>Недопустимые ключи в группе</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyGroupMetadataRules</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="68"/>
        <source>Name must contain at least five characters.</source>
        <translation>Имя должно содержать не менее пяти символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="73"/>
        <source>Email does not look like an address. Leave it empty if the group has none.</source>
        <translation>Электронная почта не похожа на адрес. Оставьте поле пустым, если у группы его нет.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="85"/>
        <source>This group is empty.</source>
        <translation>Эта группа пуста.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="95"/>
        <source>1 key</source>
        <translation>1 ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="97"/>
        <source>%1 keys</source>
        <translation>%1 ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="102"/>
        <source>1 nested group</source>
        <translation>1 вложенная группа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="105"/>
        <source>%1 nested groups</source>
        <translation>%1 вложенных групп</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="111"/>
        <source> and </source>
        <translation>и</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="115"/>
        <source>Contains %1.</source>
        <translation>Содержит %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="121"/>
        <source>1 member is no longer in your keyring.</source>
        <translation>1 участник больше не в вашей связке ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="124"/>
        <source>%1 members are no longer in your keyring.</source>
        <translation>%1 участников больше не в вашей связке ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="137"/>
        <source>1 checked key will be added to this group.</source>
        <translation>1 выбранный ключ будет добавлен в эту группу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="140"/>
        <source>%1 checked keys will be added to this group.</source>
        <translation>%1 отмеченных ключей будет добавлено в эту группу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="148"/>
        <source>Delete the key group &quot;%1&quot;?

The group is removed. The keys in it are not touched and stay in your keyring.</source>
        <translation>Удалить группу ключей «%1»?

Группа будет удалена. Ключи в ней не затрагиваются и остаются в вашей связке ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeyGroupMetadataRules.cpp" line="156"/>
        <source>It is also a member of: %1. Those groups will lose it.</source>
        <translation>Он также является членом: %1. Эти группы потеряют его.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyImportDetailDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="52"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="73"/>
        <source>Key Import Details</source>
        <translation>Подробности импорта ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="53"/>
        <source>No keys found to import</source>
        <translation>Не найдено ключей для импорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="168"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="182"/>
        <source>Considered</source>
        <translation>Рассмотрено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="169"/>
        <source>Imported</source>
        <translation>Импортировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="171"/>
        <source>Not Imported</source>
        <translation>Не импортировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="175"/>
        <source>New Revocations</source>
        <translation>Новые отзывы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="221"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="221"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="221"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="222"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="266"/>
        <source>Private</source>
        <translation>Закрытый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="269"/>
        <source>Public</source>
        <translation>Открытый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="170"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="272"/>
        <source>Unchanged</source>
        <translation>Без изменений</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="110"/>
        <source>Import Completed with Issues</source>
        <translation>Импорт завершён с ошибками</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="111"/>
        <source>%1 of %2 key(s) could not be imported.</source>
        <translation>%1 из %2 ключей не удалось импортировать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="116"/>
        <source>Import Successful</source>
        <translation>Импорт выполнен успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="118"/>
        <source>Successfully processed %1 key(s).</source>
        <translation>Успешно обработано %1 ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="120"/>
        <source>Nothing to Import</source>
        <translation>Нечего импортировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="122"/>
        <source>All %1 key(s) are already up to date.</source>
        <translation>Все %1 ключей уже актуальны.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="150"/>
        <source>Summary</source>
        <translation>Сводка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="172"/>
        <source>Private Keys Read</source>
        <translation>Прочитано закрытых ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="173"/>
        <source>Private Keys Imported</source>
        <translation>Импортировано закрытых ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="174"/>
        <source>Private Keys Unchanged</source>
        <translation>Закрытых ключей без изменений</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="275"/>
        <source>New Key</source>
        <translation>Новый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="278"/>
        <source>New Subkey</source>
        <translation>Новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="282"/>
        <source>New Signature</source>
        <translation>Новая подпись</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyImportDetailDialog.cpp" line="286"/>
        <source>New UID</source>
        <translation>Новый UID</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyList</name>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="650"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="652"/>
        <source>Refresh the key list to synchronize changes.</source>
        <translation>Обновите список ключей для синхронизации изменений.</translation>
    </message>
    <message>
        <source>Sync Public Key</source>
        <translation type="vanished">Синхронизировать открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="814"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="824"/>
        <source>Subkeys</source>
        <translation>Подключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="832"/>
        <source>No optional columns</source>
        <translation>Нет дополнительных столбцов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1513"/>
        <source>Edit Category...</source>
        <translation>Изменить категорию...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1622"/>
        <source>Refreshing Key List...</source>
        <translation>Обновление списка ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1679"/>
        <source>Import Keys</source>
        <translation>Импортировать ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1627"/>
        <source>Key List Refreshed.</source>
        <translation>Список ключей обновлён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="816"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <source>Sync public keys with the default keyserver.</source>
        <translation type="vanished">Синхронизировать открытые ключи с сервером ключей по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="379"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1492"/>
        <source>New Category...</source>
        <translation>Новая категория...</translation>
    </message>
    <message>
        <source>Sync public keys with the key server configured as the default.</source>
        <translation type="vanished">Синхронизировать открытые ключи с сервером ключей, заданным по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="654"/>
        <source>Uncheck All</source>
        <translation>Снять выделение со всех</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="655"/>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="994"/>
        <source>Uncheck all keys in the current tab.</source>
        <translation>Снять выделение со всех ключей на текущей вкладке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="657"/>
        <source>Check All</source>
        <translation>Выделить все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="658"/>
        <source>Check all keys in the current tab.</source>
        <translation>Выделить все ключи на текущей вкладке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="661"/>
        <source>Search keys by user ID, key ID, fingerprint...</source>
        <translation>Поиск ключей по идентификатору пользователя, идентификатору ключа, отпечатку...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="663"/>
        <source>Columns</source>
        <translation>Столбцы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="664"/>
        <source>Choose visible key table columns.</source>
        <translation>Выберите видимые столбцы таблицы ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="668"/>
        <source>Create a key group from checked encryption-capable keys.</source>
        <translation>Создать группу ключей из выбранных ключей с возможностью шифрования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="781"/>
        <source>Switch between key databases.</source>
        <translation>Переключение между базами данных ключей.</translation>
    </message>
    <message>
        <source>Channel %1</source>
        <translation type="vanished">Канал %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="762"/>
        <source>No key database available</source>
        <translation>Базы данных ключей недоступны</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="771"/>
        <source>Key Database</source>
        <translation>База данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="782"/>
        <source>Click to switch key databases.</source>
        <translation>Нажмите, чтобы переключить базы данных ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="820"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="822"/>
        <source>Owner Trust</source>
        <translation>Доверие владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="828"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="837"/>
        <source>Reset Column Widths</source>
        <translation>Сбросить ширины столбцов</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="996"/>
        <source>%n key(s) checked</source>
        <translation>
            <numerusform>%n ключ(ей) выбрано</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1004"/>
        <source>%n of them are not shown by the current category or search filter.</source>
        <translation>
            <numerusform>%n из них не отображаются из-за текущей категории или фильтра поиска.</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1109"/>
        <source>The key database &quot;%1&quot; is no longer available; showing the default one.</source>
        <translation>База данных ключей &quot;%1&quot; больше недоступна; отображается база данных по умолчанию.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1420"/>
        <source>Delete Category</source>
        <translation>Удалить категорию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1421"/>
        <source>Delete category &quot;%1&quot;? This removes the grouping only; the keys themselves are not affected.</source>
        <translation>Удалить категорию &quot;%1&quot;? Это удаляет только группировку; сами ключи не затрагиваются.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1498"/>
        <source>Set Colour...</source>
        <translation>Установить цвет...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1502"/>
        <source>Reset Colour</source>
        <translation>Сбросить цвет</translation>
    </message>
    <message>
        <source>Rename Category...</source>
        <translation type="vanished">Переименовать категорию...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1517"/>
        <source>Delete Category...</source>
        <translation>Удалить категорию...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1534"/>
        <source>Choose Category Colour</source>
        <translation>Выбрать цвет категории</translation>
    </message>
    <message>
        <source>New Category</source>
        <translation type="vanished">Новая категория</translation>
    </message>
    <message>
        <source>Category name:</source>
        <translation type="vanished">Имя категории:</translation>
    </message>
    <message>
        <source>Category Colour (optional)</source>
        <translation type="vanished">Цвет категории (необязательно)</translation>
    </message>
    <message>
        <source>Rename Category</source>
        <translation type="vanished">Переименовать категорию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1681"/>
        <source>You&apos;ve dropped something on the key list.
GpgFrontend will now try to import key(s).</source>
        <translation>Вы перетащили объект в список ключей.
GpgFrontend попытается импортировать ключ(и).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1686"/>
        <source>Ask before importing keys next time.</source>
        <translation>Спрашивать перед импортом ключей в следующий раз.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1815"/>
        <source>No key matches &quot;%1&quot;</source>
        <translation>Ни один ключ не соответствует &quot;%1&quot;</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="1816"/>
        <source>%n key(s) match &quot;%1&quot;</source>
        <translation>
            <numerusform>%n ключ(ей) соответствует &quot;%1&quot;</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="818"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="826"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="666"/>
        <source>New Key Group</source>
        <translation>Новая группа ключей</translation>
    </message>
    <message>
        <source>Key Databases</source>
        <translation type="vanished">Базы данных ключей</translation>
    </message>
    <message>
        <source>The key has been updated</source>
        <translation type="vanished">Ключ обновлён</translation>
    </message>
    <message>
        <source>No need to update the key</source>
        <translation type="vanished">Обновление ключа не требуется</translation>
    </message>
    <message>
        <source>Syncing Key List...</source>
        <translation type="vanished">Синхронизация списка ключей...</translation>
    </message>
    <message>
        <source>Sync [%1/%2] %3 %4</source>
        <translation type="vanished">Синхронизация [%1/%2] %3 %4</translation>
    </message>
    <message>
        <source>Key List Sync Done.</source>
        <translation type="vanished">Синхронизация списка ключей завершена.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyMgmt</name>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="86"/>
        <source>All</source>
        <translation>Все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="91"/>
        <source>Key Group</source>
        <translation>Группа ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="97"/>
        <source>Only Public Key</source>
        <translation>Только открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="105"/>
        <source>Has Private Key</source>
        <translation>Имеет закрытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="113"/>
        <source>Expiring Soon</source>
        <translation>Скоро истекает</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="119"/>
        <source>No Primary Key</source>
        <translation>Нет первичного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="129"/>
        <source>Revoked</source>
        <translation>Отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="136"/>
        <source>Expired</source>
        <translation>Срок истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="143"/>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="192"/>
        <source>KeyPair Management</source>
        <translation>Управление ключевыми парами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="249"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="259"/>
        <source>Export</source>
        <translation>Экспортировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="267"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="705"/>
        <source>Key Operations</source>
        <translation>Операции с ключами</translation>
    </message>
    <message>
        <source>Keyserver</source>
        <translation type="vanished">Сервер ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="276"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1469"/>
        <source>Category</source>
        <translation>Категория</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="331"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="331"/>
        <source>Open Key File</source>
        <translation>Открыть файл ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="337"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="342"/>
        <source>New Keypair</source>
        <translation>Новая пара ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="343"/>
        <source>Generate KeyPair</source>
        <translation>Сгенерировать пару ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="350"/>
        <source>New Subkey</source>
        <translation>Новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="351"/>
        <source>Generate Subkey For Selected KeyPair</source>
        <translation>Сгенерировать подключ для выбранной пары ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="360"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="597"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="361"/>
        <source>Import New Key From File</source>
        <translation>Импортировать новый ключ из файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="367"/>
        <source>Clipboard</source>
        <translation>Буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="368"/>
        <source>Import New Key From Clipboard</source>
        <translation>Импортировать новый ключ из буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="376"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1166"/>
        <source>Key Package</source>
        <translation>Пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="377"/>
        <source>Import Key(s) From a Key Package</source>
        <translation>Импортировать ключ(и) из пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="382"/>
        <source>Export To Clipboard</source>
        <translation>Экспортировать в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="383"/>
        <source>Export Checked Key(s) To Clipboard</source>
        <translation>Экспортировать отмеченные ключи в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="388"/>
        <source>Export As Key Package</source>
        <translation>Экспортировать как пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="389"/>
        <source>Export Checked Key(s) To a Key Package</source>
        <translation>Экспортировать отмеченные ключи в пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="394"/>
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
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="404"/>
        <source>Delete the Selected keys</source>
        <translation>Удалить выбранные ключи</translation>
    </message>
    <message>
        <source>Delete Checked Key(s)</source>
        <translation type="vanished">Удалить отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="395"/>
        <source>Export a single key in OpenSSH format to a file</source>
        <translation>Экспортировать один ключ в формате OpenSSH в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="403"/>
        <source>Delete Selected Keys</source>
        <translation>Удалить выбранные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="410"/>
        <source>Refresh Key List</source>
        <translation>Обновить список ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="411"/>
        <source>Re-read the keyring from disk</source>
        <translation>Перечитать связку ключей с диска</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="419"/>
        <source>Find Key</source>
        <translation>Найти ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="419"/>
        <source>Jump to the search box</source>
        <translation>Перейти к полю поиска</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="426"/>
        <source>Delete Checked Keys</source>
        <translation>Удалить отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="427"/>
        <source>Delete the Checked keys</source>
        <translation>Удалить отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="432"/>
        <source>Show Key Details</source>
        <translation>Показать сведения о ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="433"/>
        <source>Show Details for this Key</source>
        <translation>Показать сведения об этом ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="438"/>
        <source>Set Owner Trust Level</source>
        <translation>Установить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="439"/>
        <source>Set how much you trust this key to certify others</source>
        <translation>Установить степень доверия ключу для сертификации других</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="480"/>
        <source>Copy Fingerprint</source>
        <translation>Копировать отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="481"/>
        <source>Copy this key&apos;s full fingerprint to the clipboard</source>
        <translation>Скопировать полный отпечаток этого ключа в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="485"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="488"/>
        <source>Copy Key ID</source>
        <translation>Копировать идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="489"/>
        <source>Copy this key&apos;s ID to the clipboard</source>
        <translation>Скопировать идентификатор этого ключа в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="493"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="497"/>
        <source>Copy Email</source>
        <translation>Копировать электронную почту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="498"/>
        <source>Copy this key&apos;s email address to the clipboard</source>
        <translation>Скопировать адрес электронной почты этого ключа в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="502"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="506"/>
        <source>Copy Public Key Block</source>
        <translation>Копировать блок открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="507"/>
        <source>Copy this key&apos;s armored public key block to the clipboard, ready to paste or send</source>
        <translation>Копировать ASCII-блок открытого ключа в буфер обмена для вставки или отправки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="517"/>
        <source>Certify Key...</source>
        <translation>Подписать ключ...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="518"/>
        <source>Sign this key&apos;s identity with your key</source>
        <translation>Подписать личность этого ключа вашим ключом</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="527"/>
        <source>Set Expiry...</source>
        <translation>Установить срок действия...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="527"/>
        <source>Change when this key expires</source>
        <translation>Изменить срок действия этого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="539"/>
        <source>Export Public Key...</source>
        <translation>Экспортировать открытый ключ…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="540"/>
        <source>Save this key&apos;s public half to a file you can send</source>
        <translation>Сохранить открытую часть этого ключа в файл для отправки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="549"/>
        <source>Export Private Key...</source>
        <translation>Экспортировать закрытый ключ…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="550"/>
        <source>Save this key&apos;s private half to a file — keep it to yourself</source>
        <translation>Сохранить закрытую часть этого ключа в файл — храните в тайне</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="559"/>
        <source>Generate Revocation Certificate...</source>
        <translation>Создать сертификат отзыва...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="560"/>
        <source>Create a certificate that can revoke this key, to store somewhere safe in case you ever lose control of it</source>
        <translation>Создать сертификат отзыва этого ключа для безопасного хранения на случай потери контроля над ним</translation>
    </message>
    <message>
        <source>Search Keyserver...</source>
        <translation type="vanished">Поиск на сервере ключей...</translation>
    </message>
    <message>
        <source>Search a keyserver and import keys</source>
        <translation type="vanished">Поиск на сервере ключей и импорт ключей</translation>
    </message>
    <message>
        <source>Publish Key to Keyserver...</source>
        <translation type="vanished">Опубликовать ключ на сервере ключей...</translation>
    </message>
    <message>
        <source>Refresh Selected from Keyserver</source>
        <translation type="vanished">Обновить выбранные с сервера ключей</translation>
    </message>
    <message>
        <source>Fetch the latest copy of these key(s) from the key server, picking up new signatures and revocations</source>
        <translation type="vanished">Загрузить последнюю версию этих ключей с сервера ключей, включая новые подписи и отзывы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="572"/>
        <source>Set Owner Trust for Checked Keys...</source>
        <translation>Установить доверие владельца для отмеченных ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="573"/>
        <source>Give every checked key the same owner trust level</source>
        <translation>Установить одинаковый уровень доверия владельца для всех отмеченных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="578"/>
        <source>Extend Expiry of Checked Keys...</source>
        <translation>Продлить срок действия отмеченных ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="579"/>
        <source>Push back the expiry date of every checked private key in one step</source>
        <translation>Продлить срок действия всех отмеченных закрытых ключей за один шаг</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="585"/>
        <source>Back Up All Private Keys...</source>
        <translation>Резервное копирование всех закрытых ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="586"/>
        <source>Export every private key in this keyring to a single key package</source>
        <translation>Экспортировать все закрытые ключи из этой связки в единый пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="601"/>
        <source>Edit</source>
        <translation>Правка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="610"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="668"/>
        <source>Key</source>
        <translation>Ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="611"/>
        <source>Generate Key</source>
        <translation>Сгенерировать ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="615"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="684"/>
        <source>Import Key</source>
        <translation>Импорт ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="622"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="691"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="692"/>
        <source>Export Key</source>
        <translation>Экспортировать ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="638"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="718"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="643"/>
        <source>Operations</source>
        <translation>Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="654"/>
        <source>Bulk</source>
        <translation>Массовые операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="685"/>
        <source>Import key</source>
        <translation>Импортировать ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="706"/>
        <source>Certify, set expiry, add a subkey, set trust, revoke</source>
        <translation>Сертифицировать, установить срок действия, добавить подключ, задать доверие, отозвать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="719"/>
        <source>Delete the selected or the checked keys</source>
        <translation>Удалить выбранные или отмеченные ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="725"/>
        <source>View</source>
        <translation>Просмотр</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="841"/>
        <source>Deleting Keys</source>
        <translation>Удаление ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="842"/>
        <source>Are you sure that you want to delete the following keys?</source>
        <translation>Вы уверены, что хотите удалить следующие ключи?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="844"/>
        <source>The action can not be undone.</source>
        <translation>Это действие нельзя отменить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="901"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="913"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1119"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1130"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1143"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1175"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1183"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1199"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1207"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1236"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="930"/>
        <source>%1 is empty</source>
        <translation>%1 пуст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="934"/>
        <source>%1 copied to clipboard</source>
        <translation>%1 скопирован в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="989"/>
        <source>Nothing to Extend</source>
        <translation>Нечего продлевать</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="990"/>
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
        <source>Publish Key to Keyserver</source>
        <translation type="vanished">Опубликовать ключ на сервере ключей</translation>
    </message>
    <message>
        <source>You are about to upload the following public key(s) to the default keyserver:</source>
        <translation type="vanished">Вы собираетесь загрузить следующие открытые ключи на сервер ключей по умолчанию:</translation>
    </message>
    <message>
        <source>Publication is &lt;b&gt;permanent and public&lt;/b&gt;: the key(s) cannot be removed from most keyservers once uploaded. Only the public part is uploaded, never your private key.</source>
        <translation type="vanished">Публикация является &lt;b&gt;постоянной и общедоступной&lt;/b&gt;: после загрузки ключи невозможно удалить с большинства серверов ключей. Загружается только открытая часть, ваш закрытый ключ никогда не передаётся.</translation>
    </message>
    <message>
        <source>Do you want to proceed?</source>
        <translation type="vanished">Продолжить?</translation>
    </message>
    <message>
        <source>Upload Failed</source>
        <translation type="vanished">Ошибка загрузки</translation>
    </message>
    <message>
        <source>The public key was uploaded to the key server.</source>
        <translation type="vanished">Открытый ключ был загружен на сервер ключей.</translation>
    </message>
    <message>
        <source>The public key was uploaded to %1.</source>
        <translation type="vanished">Открытый ключ был загружен на %1.</translation>
    </message>
    <message>
        <source>Fingerprint: %1</source>
        <translation type="vanished">Отпечаток: %1</translation>
    </message>
    <message>
        <source>Upload Complete</source>
        <translation type="vanished">Загрузка завершена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="942"/>
        <source>Undefined</source>
        <translation>Не определено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="942"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="942"/>
        <source>Marginal</source>
        <translation>Предел.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="942"/>
        <source>Full</source>
        <translation>Полный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="943"/>
        <source>Ultimate</source>
        <translation>Абсолютный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="947"/>
        <source>Modify Owner Trust Level</source>
        <translation>Изменить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="948"/>
        <source>Trust level to apply to %1 checked key(s):</source>
        <translation>Уровень доверия для %1 выбранного ключа(ей):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="967"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1042"/>
        <source>Partially Failed</source>
        <translation>Частично не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="968"/>
        <source>Failed to set owner trust on %1 key(s).</source>
        <translation>Не удалось установить доверие владельца для %1 ключа(ей).</translation>
    </message>
    <message>
        <source>Please check some private keys before doing this operation.</source>
        <translation type="vanished">Пожалуйста, выберите несколько закрытых ключей перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="997"/>
        <source>Extend Expiry of Checked Keys</source>
        <translation>Продлить срок действия выбранных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1000"/>
        <source>New expiry date to apply to %1 private key(s):</source>
        <translation>Новая дата окончания срока действия для %1 закрытого ключа(ей):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1008"/>
        <source>Never expires</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1027"/>
        <source>Extending Expiry</source>
        <translation>Продление срока действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1043"/>
        <source>Failed to update expiry on %1 key(s).</source>
        <translation>Не удалось обновить срок действия для %1 ключа(ей).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1061"/>
        <source>No Private Keys</source>
        <translation>Нет закрытых ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1062"/>
        <source>There are no private keys in this keyring to back up.</source>
        <translation>В этой связке ключей нет закрытых ключей для резервного копирования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1075"/>
        <source>private key(s) backed up</source>
        <translation>Закрытые ключи сохранены в резервной копии</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1213"/>
        <source>Enter PIN</source>
        <translation>Введите PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1214"/>
        <source>Please enter PIN to decrypt the Key:</source>
        <translation>Введите PIN для расшифровки ключа:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1468"/>
        <source>Category (%1 keys)</source>
        <translation>Категория (%1 ключ(ей))</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1480"/>
        <source>Remove From This Category</source>
        <translation>Удалить из этой категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1516"/>
        <source>New Category...</source>
        <translation>Новая категория...</translation>
    </message>
    <message>
        <source>New Category</source>
        <translation type="vanished">Новая категория</translation>
    </message>
    <message>
        <source>Category name:</source>
        <translation type="vanished">Название категории:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1547"/>
        <source>%1 keys · %2 private · %3 expiring soon · %4 expired or revoked</source>
        <translation>%1 ключей · %2 закрытых · %3 скоро истекающих · %4 истекших или отозванных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1554"/>
        <source>%1 key(s) expire within %2 days. See the &quot;Expiring Soon&quot; tab.</source>
        <translation>%1 ключ(ей) истекает в течение %2 дней. Смотрите вкладку «Скоро истекают».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1558"/>
        <source>No key expires within the next %1 days.</source>
        <translation>Ни один ключ не истекает в ближайшие %1 дня(ей).</translation>
    </message>
    <message>
        <source>Forbidden</source>
        <translation type="vanished">Запрещено</translation>
    </message>
    <message>
        <source>Upload the checked public key(s) to the key server configured as the default</source>
        <translation type="vanished">Загрузить выбранные открытые ключи на сервер ключей, настроенный по умолчанию</translation>
    </message>
    <message>
        <source>Please check some keys before doing this operation.</source>
        <translation type="vanished">Отметьте несколько ключей перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="875"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1158"/>
        <source>key(s) exported</source>
        <translation>ключ(и) экспортированы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="891"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1109"/>
        <source>Exporting</source>
        <translation>Экспорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="902"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="914"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1120"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1131"/>
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
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1090"/>
        <source>Invalid Operation</source>
        <translation>Недопустимая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1091"/>
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
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1144"/>
        <source>This key may not be able to export as OpenSSH format. Please check the key-size of the subkey(s) used to sign.</source>
        <translation>Этот ключ может быть невозможно экспортировать в формате OpenSSH. Проверьте размер подключей, используемых для подписи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1153"/>
        <source>Export OpenSSH Key To File</source>
        <translation>Экспортировать ключ OpenSSH в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1154"/>
        <source>OpenSSH Public Key Files</source>
        <translation>Файлы открытых ключей OpenSSH</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1166"/>
        <source>Import Key Package</source>
        <translation>Импортировать пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1176"/>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1200"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Невозможно открыть этот файл. Убедитесь, что это обычный файл и он доступен для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1184"/>
        <source>The target file is too large for a key package.</source>
        <translation>Целевой файл слишком большой для пакета ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1189"/>
        <source>Import Key Package Passphrase File</source>
        <translation>Импортировать файл пароля пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1190"/>
        <source>Key Package Passphrase File</source>
        <translation>Файл пароля пакета ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1208"/>
        <source>The target file is too large for a key package passphrase.</source>
        <translation>Целевой файл слишком большой для пароля пакета ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/KeyMgmt.cpp" line="1242"/>
        <source>key(s) imported</source>
        <translation>ключ(и) импортированы</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyNewUIDDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="52"/>
        <source>Add a new User ID</source>
        <translation>Добавить новый идентификатор пользователя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="59"/>
        <source>A User ID pairs a name with an optional email and comment. The name is required.</source>
        <translation>Идентификатор пользователя связывает имя с необязательными электронной почтой и комментарием. Имя обязательно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="65"/>
        <source>Full name</source>
        <translation>Полное имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="68"/>
        <source>name@example.com</source>
        <translation>name@example.com</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="71"/>
        <source>Optional comment</source>
        <translation>Необязательный комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="77"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="78"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="79"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="86"/>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="97"/>
        <source>The new User ID will be set as the primary User ID.</source>
        <translation>Новый идентификатор пользователя будет установлен как основной.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="146"/>
        <source>Enter a name for the User ID.</source>
        <translation>Введите имя для идентификатора пользователя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="159"/>
        <source>Please give a valid email address.</source>
        <translation>Укажите действительный адрес электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="161"/>
        <source>Ready to create the User ID.</source>
        <translation>Готов к созданию идентификатора пользователя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="203"/>
        <source>Successful Operation</source>
        <translation>Операция выполнена успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="204"/>
        <source>Successfully added a new UID.</source>
        <translation>Новый UID успешно добавлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="212"/>
        <source>Operation Failed</source>
        <translation>Операция не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="213"/>
        <source>An error occurred during the operation.</source>
        <translation>Во время операции произошла ошибка.</translation>
    </message>
    <message>
        <source>Notice: The New UID Created will be set as Primary.</source>
        <translation type="vanished">Примечание: созданный новый UID будет установлен как основной.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="123"/>
        <source>Create New UID</source>
        <translation>Создать новый UID</translation>
    </message>
    <message>
        <source>Name must contain at least five characters.</source>
        <translation type="vanished">Имя должно содержать не менее пяти символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="153"/>
        <source>Name and comment must not contain the characters &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; or control characters.</source>
        <translation>Имя и комментарий не должны содержать символы &apos;(&apos;, &apos;)&apos;, &apos;&lt;&apos;, &apos;&gt;&apos; или управляющие символы.</translation>
    </message>
    <message>
        <source>Please give a email address.</source>
        <translation type="vanished">Укажите адрес электронной почты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyNewUIDDialog.cpp" line="217"/>
        <source>Creating UID</source>
        <translation>Создание UID</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairDetailTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="70"/>
        <source>Owner</source>
        <translation>Владелец</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="71"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="72"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="73"/>
        <source>Additional UIDs</source>
        <translation>Дополнительные UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="113"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="114"/>
        <source>Email Address</source>
        <translation>Адрес электронной почты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="115"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="128"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="143"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="147"/>
        <source>Algorithm Detail</source>
        <translation>Подробности алгоритма</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="152"/>
        <source>Key Format Version</source>
        <translation>Версия формата ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="160"/>
        <source>Key Size</source>
        <translation>Размер ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="164"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="169"/>
        <source>Owner Trust Level</source>
        <translation>Уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="177"/>
        <source>Create Date (Local Time)</source>
        <translation>Дата создания (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="183"/>
        <source>Expires on (Local Time)</source>
        <translation>Истекает (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="191"/>
        <source>Last Update (Local Time)</source>
        <translation>Последнее обновление (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="196"/>
        <source>Primary Key Existence</source>
        <translation>Наличие первичного ключа</translation>
    </message>
    <message>
        <source>Notice: The public key has been published on the key server.</source>
        <translation type="vanished">Уведомление: открытый ключ был опубликован на сервере ключей.</translation>
    </message>
    <message>
        <source>Notice: The public key has been published on %1.</source>
        <translation type="vanished">Уведомление: открытый ключ был опубликован на %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="131"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="228"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="230"/>
        <source>copy fingerprint to clipboard</source>
        <translation>Копировать отпечаток в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="280"/>
        <source>Exists</source>
        <translation>Существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="280"/>
        <source>Not Exists</source>
        <translation>Не существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="303"/>
        <source>Certificate</source>
        <translation>Сертификат</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="305"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="306"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="307"/>
        <source>Auth</source>
        <translation>Аутентификация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="333"/>
        <source>Never Expire</source>
        <translation>Без срока действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="352"/>
        <source>No Data</source>
        <translation>Нет данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="366"/>
        <source>Warning: The primary key has expired.</source>
        <translation>Предупреждение: срок действия первичного ключа истёк.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="369"/>
        <source>Warning: The primary key has been revoked.</source>
        <translation>Предупреждение: первичный ключ был отозван.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairDetailTab.cpp" line="372"/>
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
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="73"/>
        <source>General Operations</source>
        <translation>Общие операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="79"/>
        <source>Export Public Key</source>
        <translation>Экспортировать открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="87"/>
        <source>Export Private Key</source>
        <translation>Экспортировать закрытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="98"/>
        <source>Modify Expiration Datetime (Primary Key)</source>
        <translation>Изменить дату истечения (первичный ключ)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="105"/>
        <source>Modify Password</source>
        <translation>Изменить пароль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="114"/>
        <source>Revoke Certificate Operation</source>
        <translation>Операция с сертификатом отзыва</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="125"/>
        <source>Set Owner Trust Level</source>
        <translation>Установить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="161"/>
        <source>Export Full Secret Key</source>
        <translation>Экспортировать полный секретный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="169"/>
        <source>Export Shortest Secret Key</source>
        <translation>Экспортировать минимальный секретный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="177"/>
        <source>Generate Revoke Certificate</source>
        <translation>Сгенерировать сертификат отзыва</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="182"/>
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
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="247"/>
        <source>Revocation Certificates</source>
        <translation>Сертификаты отзыва</translation>
    </message>
    <message>
        <source>Generate revocation certificate</source>
        <translation type="vanished">Сгенерировать сертификат отзыва</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="234"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="246"/>
        <source>Import Key Revocation Certificate</source>
        <translation>Импортировать сертификат отзыва ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="235"/>
        <source>You are about to import the</source>
        <translation>Вы собираетесь импортировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="236"/>
        <source>REVOCATION CERTIFICATE</source>
        <translation>СЕРТИФИКАТ ОТЗЫВА</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="237"/>
        <source>A successful import will result in the key being irreversibly revoked.</source>
        <translation>Успешный импорт приведёт к необратимому отзыву ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="239"/>
        <source>Do you REALLY want to execute this operation?</source>
        <translation>Вы ДЕЙСТВИТЕЛЬНО хотите выполнить эту операцию?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="255"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="264"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="272"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="256"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="273"/>
        <source>Cannot open this file. Please make sure that this is a regular file and it&apos;s readable.</source>
        <translation>Невозможно открыть этот файл. Убедитесь, что это обычный файл и он доступен для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairOperaTab.cpp" line="265"/>
        <source>The target file is too large for a key revocation certificate.</source>
        <translation>Целевой файл слишком большой для сертификата отзыва ключа.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairPhotosTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="56"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="56"/>
        <source>Flags</source>
        <translation>Флаги</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="56"/>
        <source>Date</source>
        <translation>Дата</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="56"/>
        <source>Size</source>
        <translation>Размер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="56"/>
        <source>Thumbnail</source>
        <translation>Миниатюра</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="106"/>
        <source>List of Photo IDs (%1)</source>
        <translation>Список фото-идентификаторов (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairPhotosTab.cpp" line="107"/>
        <source>Photo Viewer</source>
        <translation>Просмотр фотографий</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairSubkeyTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="161"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="279"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="171"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="279"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="176"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="280"/>
        <source>Algorithm Detail</source>
        <translation>Подробности алгоритма</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="182"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="279"/>
        <source>Key Size</source>
        <translation>Размер ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="187"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="192"/>
        <source>Expires On (Local Time)</source>
        <translation>Истекает (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="199"/>
        <source>Create Date (Local Time)</source>
        <translation>Дата создания (местное время)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="203"/>
        <source>Existence</source>
        <translation>Наличие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="214"/>
        <source>Key in Smart Card</source>
        <translation>Ключ на смарт-карте</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="219"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="86"/>
        <source>List of the primary key and subkey(s)</source>
        <translation>Список первичного ключа и подключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="87"/>
        <source>Detail of Selected Primary Key/Subkey</source>
        <translation>Сведения о выбранном первичном ключе/подключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="91"/>
        <source>New Subkey</source>
        <translation>Новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="94"/>
        <source>Add ADSK(s)</source>
        <translation>Добавить ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="166"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="279"/>
        <source>Key Type</source>
        <translation>Тип ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="209"/>
        <source>Revoked</source>
        <translation>Отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="113"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="476"/>
        <source>Export Subkey</source>
        <translation>Экспортировать подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="280"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="283"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="306"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="481"/>
        <source>Primary Key</source>
        <translation>Первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="306"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="482"/>
        <source>Subkey</source>
        <translation>Подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="307"/>
        <source>ADSK</source>
        <translation>ADSK</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="333"/>
        <source>Never Expire</source>
        <translation>Без срока действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="455"/>
        <source>Never Expires</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="417"/>
        <source>Exists</source>
        <translation>Существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="119"/>
        <source>Exporting subkeys is not supported by the current OpenPGP backend.</source>
        <translation>Экспорт подключей не поддерживается текущим бэкендом OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="418"/>
        <source>Not Exists</source>
        <translation>Не существует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="421"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="484"/>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="421"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="484"/>
        <source>No</source>
        <translation>Нет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="475"/>
        <source>Export Primary Key</source>
        <translation>Экспортировать первичный ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="494"/>
        <source>Edit Expire Date</source>
        <translation>Изменить дату истечения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="498"/>
        <source>Export</source>
        <translation>Экспорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="502"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="506"/>
        <source>Revoke</source>
        <translation>Отозвать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="510"/>
        <source>Change Passphrase</source>
        <translation>Изменить парольную фразу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="514"/>
        <source>Move to Card</source>
        <translation>Переместить на карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="636"/>
        <source>Exporting Subkey</source>
        <translation>Экспорт подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="637"/>
        <source>You are about to export a private subkey.</source>
        <translation>Вы собираетесь экспортировать закрытый подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="638"/>
        <source>While subkeys are less critical than the primary key, they should still be handled with care.</source>
        <translation>Хотя подключи менее критичны, чем первичный ключ, с ними всё равно следует обращаться осторожно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="641"/>
        <source>Do you want to proceed with exporting this subkey?</source>
        <translation>Хотите продолжить экспорт этого подключа?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="668"/>
        <source>Export Key To File</source>
        <translation>Экспортировать ключ в файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="669"/>
        <source>Key Files</source>
        <translation>Файлы ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="674"/>
        <source>Export Error</source>
        <translation>Ошибка экспорта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="675"/>
        <source>Couldn&apos;t open %1 for writing</source>
        <translation>Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="685"/>
        <source>&lt;h3&gt;You are about to delete the subkey:&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;This action is irreversible. Please confirm.</source>
        <translation>&lt;h3&gt;Вы собираетесь удалить подключ:&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;Идентификатор:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Это действие необратимо. Пожалуйста, подтвердите.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="691"/>
        <source>Delete Subkey Confirmation</source>
        <translation>Подтверждение удаления подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="706"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="791"/>
        <source>Illegal Operation</source>
        <translation>Недопустимая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="707"/>
        <source>Cannot delete the primary key or an invalid subkey.</source>
        <translation>Невозможно удалить первичный ключ или недопустимый подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="715"/>
        <source>Operation Failed</source>
        <translation>Операция не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="716"/>
        <source>The selected subkey could not be deleted. Please check your permissions or try again.</source>
        <translation>Не удалось удалить выбранный подключ. Проверьте права доступа или попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="722"/>
        <source>Operation Successful</source>
        <translation>Операция выполнена успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="723"/>
        <source>The subkey with KeyID %1 has been successfully deleted.</source>
        <translation>Подключ с идентификатором %1 успешно удалён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="768"/>
        <source>&lt;h3&gt;Revoke Subkey Confirmation&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;KeyID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Revoking a subkey will make it permanently unusable. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br /&gt;Are you sure you want to revoke this subkey?</source>
        <translation>&lt;h3&gt;Подтверждение отзыва подключа&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;Идентификатор:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Отзыв подключа сделает его навсегда непригодным для использования. Это действие &lt;b&gt;необратимо&lt;/b&gt;.&lt;br /&gt;Вы уверены, что хотите отозвать этот подключ?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="775"/>
        <source>Revoke Subkey</source>
        <translation>Отозвать подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="792"/>
        <source>Cannot revoke the primary key or an invalid subkey.</source>
        <translation>Невозможно отозвать первичный ключ или недопустимый подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="797"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Без причины.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="797"/>
        <source>1 -&gt; This key is no more safe.</source>
        <translation>1 -&gt; Этот ключ больше не безопасен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="798"/>
        <source>2 -&gt; Key is outdated.</source>
        <translation>2 -&gt; Ключ устарел.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="798"/>
        <source>3 -&gt; Key is no longer used</source>
        <translation>3 -&gt; Ключ больше не используется</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="810"/>
        <source>Revocation Failed</source>
        <translation>Отзыв не удался</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="811"/>
        <source>Failed to revoke the subkey. Please try again.</source>
        <translation>Не удалось отозвать подключ. Пожалуйста, попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="814"/>
        <source>Revocation Successful</source>
        <translation>Отзыв выполнен успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairSubkeyTab.cpp" line="815"/>
        <source>The subkey has been successfully revoked.</source>
        <translation>Подключ успешно отозван.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeyPairUIDTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="65"/>
        <source>New UID</source>
        <translation>Новый UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="80"/>
        <source>UIDs</source>
        <translation>Идентификаторы пользователя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="91"/>
        <source>Signature of Selected UID</source>
        <translation>Подпись выбранного UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="143"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="166"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="143"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="166"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="143"/>
        <source>Comment</source>
        <translation>Комментарий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="143"/>
        <source>Index</source>
        <translation>Индекс</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="166"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="166"/>
        <source>Create Date</source>
        <translation>Дата создания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="167"/>
        <source>Expired Date</source>
        <translation>Дата истечения срока</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="272"/>
        <source>Never Expires</source>
        <translation>Бессрочный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="305"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="315"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="470"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="479"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="525"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="546"/>
        <source>Invalid Operation</source>
        <translation>Недопустимая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="306"/>
        <source>This is the only UID of a version 4 key. OpenPGP allows a key without UIDs, but a version 4 key keeps its usage, expiry and preferences in its UID self-signatures, so GpgFrontend could no longer use this key to sign or encrypt. Add another UID first if you want to replace this one.</source>
        <translation>Это единственный UID ключа версии 4. OpenPGP допускает ключи без UID, но ключ версии 4 хранит назначение, срок действия и предпочтения в самоподписях своих UID, поэтому GpgFrontend больше не сможет использовать этот ключ для подписи или шифрования. Если хотите заменить этот UID, сначала добавьте другой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="324"/>
        <source>Are you sure that you want to delete the following UID?</source>
        <translation>Вы уверены, что хотите удалить следующий UID?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="345"/>
        <source>Successfully deleted the UID.</source>
        <translation>UID успешно удалён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="349"/>
        <source>Deleting UID</source>
        <translation>Удаление UID</translation>
    </message>
    <message>
        <source>Successfully set the Primary UID.</source>
        <translation type="vanished">Основной UID успешно установлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="389"/>
        <source>Setting Primary UID</source>
        <translation>Назначение основного UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="532"/>
        <source>&lt;h3&gt;Revoke UID Confirmation&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;UID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Revoking a UID will make it permanently unusable. This action is &lt;b&gt;irreversible&lt;/b&gt;.&lt;br /&gt;Are you sure you want to revoke this UID?</source>
        <translation>&lt;h3&gt;Подтверждение отзыва UID&lt;/h3&gt;&lt;br /&gt;&lt;b&gt;UID:&lt;/b&gt; %1&lt;br /&gt;&lt;br /&gt;Отзыв UID сделает его навсегда непригодным для использования. Это действие &lt;b&gt;необратимо&lt;/b&gt;.&lt;br /&gt;Вы уверены, что хотите отозвать этот UID?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="552"/>
        <source>0 -&gt; No Reason.</source>
        <translation>0 -&gt; Без причины.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="552"/>
        <source>4 -&gt; User ID is no longer valid.</source>
        <translation>4 -&gt; Идентификатор пользователя больше не действителен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="567"/>
        <source>Revocation Failed</source>
        <translation>Отзыв не удался</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="568"/>
        <source>Failed to revoke the UID. Please try again.</source>
        <translation>Не удалось отозвать UID. Пожалуйста, попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="572"/>
        <source>Revocation Successful</source>
        <translation>Отзыв выполнен успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="573"/>
        <source>The UID has been successfully revoked.</source>
        <translation>UID успешно отозван.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="577"/>
        <source>Revoking UID</source>
        <translation>Отзыв UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="344"/>
        <source>Successful Operation</source>
        <translation>Успешная операция</translation>
    </message>
    <message>
        <source>Successfully added a new UID.</source>
        <translation type="vanished">Новый UID успешно добавлен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="338"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="382"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="501"/>
        <source>Operation Failed</source>
        <translation>Операция не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="383"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="502"/>
        <source>An error occurred during the operation.</source>
        <translation>Произошла ошибка при выполнении операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="316"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="547"/>
        <source>Cannot delete the Primary UID.</source>
        <translation>Невозможно удалить основной UID.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="321"/>
        <source>Deleting UIDs</source>
        <translation>Удаление UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="326"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="369"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="495"/>
        <source>The action can not be undone.</source>
        <translation>Это действие нельзя отменить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="339"/>
        <source>An error occurred during the delete %1 operation.</source>
        <translation>Произошла ошибка при удалении %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="366"/>
        <source>Set Primary UID</source>
        <translation>Установить основной UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="367"/>
        <source>Are you sure that you want to set the Primary UID to?</source>
        <translation>Вы уверены, что хотите установить основной UID?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="406"/>
        <source>Set As Primary</source>
        <translation>Установить как основной</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="409"/>
        <source>Sign UID</source>
        <translation>Подписать UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="412"/>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="539"/>
        <source>Revoke UID</source>
        <translation>Отозвать UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="415"/>
        <source>Delete UID</source>
        <translation>Удалить UID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="526"/>
        <source>Please select one UID before doing this operation.</source>
        <translation>Выберите один UID перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="459"/>
        <source>Delete(Revoke) Key Signature</source>
        <translation>Удалить (отозвать) подпись ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="471"/>
        <source>Please select one Key Signature before doing this operation.</source>
        <translation>Выберите одну подпись ключа перед выполнением этой операции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="480"/>
        <source>To delete the signature, you need to have its corresponding public key in the local database.</source>
        <translation>Для удаления подписи необходимо иметь соответствующий открытый ключ в локальной базе данных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="490"/>
        <source>Deleting Key Signature</source>
        <translation>Удаление подписи ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyPairUIDTab.cpp" line="492"/>
        <source>Are you sure that you want to delete the following signature?</source>
        <translation>Вы уверены, что хотите удалить следующую подпись?</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::KeySetExpireDateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="123"/>
        <source>1 year</source>
        <translation>1 год</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="123"/>
        <source>%1 years</source>
        <translation>%1 лет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="126"/>
        <source>1 month</source>
        <translation>1 месяц</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="126"/>
        <source>%1 months</source>
        <translation>%1 месяцев</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="130"/>
        <source>1 day</source>
        <translation>1 день</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="130"/>
        <source>%1 days</source>
        <translation>%1 дней</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="133"/>
        <source>, </source>
        <translation>,</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="150"/>
        <source>Subkey Expiration Date</source>
        <translation>Дата истечения подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="151"/>
        <source>Key Expiration Date</source>
        <translation>Дата истечения ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="158"/>
        <source>The subkey can no longer sign or encrypt after this moment. You can extend it again later.</source>
        <translation>После этого момента подключ больше не сможет подписывать или зашифровывать. Вы сможете продлить его позже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="160"/>
        <source>The key can no longer sign or encrypt after this moment. You can extend it again later.</source>
        <translation>После этого момента ключ больше не сможет подписывать или зашифровывать. Вы сможете продлить его позже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="199"/>
        <source>Validity Period</source>
        <translation>Срок действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="200"/>
        <source>Expires On</source>
        <translation>Истекает</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="236"/>
        <source>Currently set to never expire.</source>
        <translation>Сейчас установлено без срока действия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="241"/>
        <source>Currently expired since %1.</source>
        <translation>Срок истёк с %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="243"/>
        <source>Currently expires %1 · %2 left</source>
        <translation>Истекает %1 · осталось %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="257"/>
        <source>Modify Subkey Expiration Date</source>
        <translation>Изменить дату истечения подключя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="258"/>
        <source>Modify Key Expiration Date</source>
        <translation>Изменить дату истечения ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="281"/>
        <source>Will never expire.</source>
        <translation>Никогда не истечёт.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="283"/>
        <source>The expiration date must be in the future.</source>
        <translation>Дата истечения должна быть в будущем.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="285"/>
        <source>Valid for %1 — until %2</source>
        <translation>Действителен %1 — до %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="332"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="333"/>
        <source>The expire date of the key pair has been updated.</source>
        <translation>Дата истечения пары ключей обновлена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="346"/>
        <source>Failure</source>
        <translation>Неудача</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeySetExpireDateDialog.cpp" line="347"/>
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
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="59"/>
        <source>Signers</source>
        <translation>Подписанты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="100"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="110"/>
        <source>Sign For Key&apos;s UID(s)</source>
        <translation>Подписать UID ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="132"/>
        <source>Unsuccessful Operation</source>
        <translation>Неуспешная операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="133"/>
        <source>Signature operation failed for UID %1</source>
        <translation>Не удалось подписать UID %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="137"/>
        <source>Operation Complete</source>
        <translation>Операция завершена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/keypair_details/KeyUIDSignDialog.cpp" line="138"/>
        <source>The signature operation of the UID is complete</source>
        <translation>Операция подписи UID завершена</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::LogViewDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="92"/>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="96"/>
        <source>Application Logs</source>
        <translation>Журналы приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="102"/>
        <source>No entries</source>
        <translation>Нет записей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="112"/>
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
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="280"/>
        <source>Save Failed</source>
        <translation>Ошибка сохранения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="270"/>
        <source>Unable to open file for writing.</source>
        <translation>Не удалось открыть файл для записи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="281"/>
        <source>Failed to save the log file.</source>
        <translation>Не удалось сохранить файл журнала.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="285"/>
        <source>Saved</source>
        <translation>Сохранено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/LogViewDialog.cpp" line="286"/>
        <source>Logs have been saved successfully.</source>
        <translation>Журналы были успешно сохранены.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::Lua::NativeSettingsPage</name>
    <message>
        <location filename="../../../../src/ui/lua/LuaMounts.cpp" line="192"/>
        <source>The module that provided this page is no longer active.</source>
        <translation>Модуль, предоставивший эту страницу, больше не активен.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::MainWindow</name>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="175"/>
        <source>GUI Pinentry Not Found</source>
        <translation>Графическая программа ввода PIN не найдена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="176"/>
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
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="202"/>
        <source>Add To Category</source>
        <translation>Добавить в категорию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="287"/>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="290"/>
        <source>A critical error occurred while loading GpgFrontend.</source>
        <translation>Критическая ошибка при загрузке GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="289"/>
        <source>Loading Failed</source>
        <translation>Загрузка не удалась</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="470"/>
        <source>Remove From This Category</source>
        <translation>Удалить из этой категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="504"/>
        <source>New Category...</source>
        <translation>Новая категория...</translation>
    </message>
    <message>
        <source>New Category</source>
        <translation type="vanished">Новая категория</translation>
    </message>
    <message>
        <source>Category name:</source>
        <translation type="vanished">Имя категории:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="537"/>
        <source>Name: %1</source>
        <translation>Имя: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="538"/>
        <source>Path: %1</source>
        <translation>Путь: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="551"/>
        <source>Invalid Key Databases</source>
        <translation>Недопустимые базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="556"/>
        <source>Detected %1 invalid key database(s).</source>
        <translation>Обнаружено %1 недопустимых баз данных ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindow.cpp" line="559"/>
        <source>The application cannot load these databases. Please review the details below and fix the issues in the GnuPG Controller.</source>
        <translation>Приложение не может загрузить эти базы данных. Просмотрите подробности ниже и устраните проблемы в контроллере GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="109"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="251"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="290"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="269"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="316"/>
        <source>Invalid KeyPair</source>
        <translation>Недопустимая пара ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="332"/>
        <source>The selected keypair cannot be used for encryption.</source>
        <translation>Выбранная пара ключей не может использоваться для шифрования.</translation>
    </message>
    <message>
        <source>Instant Messaging</source>
        <translation type="vanished">Мгновенные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="760"/>
        <source>Encoding</source>
        <translation>Кодировка</translation>
    </message>
    <message>
        <source>Container Format</source>
        <translation type="vanished">Формат контейнера</translation>
    </message>
    <message>
        <source>Message Book</source>
        <translation type="vanished">Книга сообщений</translation>
    </message>
    <message>
        <source>Shared phrase (Argon2id)</source>
        <translation type="vanished">Общая фраза (Argon2id)</translation>
    </message>
    <message>
        <source>Default — no shared phrase set</source>
        <translation type="vanished">По умолчанию — общая фраза не задана</translation>
    </message>
    <message>
        <source>Book Fingerprint</source>
        <translation type="vanished">Отпечаток книги</translation>
    </message>
    <message>
        <source>Set a Phrase</source>
        <translation type="vanished">Задать фразу</translation>
    </message>
    <message>
        <source>Settings → Instant Messaging</source>
        <translation type="vanished">Настройки → Мгновенные сообщения</translation>
    </message>
    <message>
        <source>OpenPGP Payload</source>
        <translation type="vanished">Полезная нагрузка OpenPGP</translation>
    </message>
    <message>
        <source>%1 bytes</source>
        <translation type="vanished">%1 байт</translation>
    </message>
    <message>
        <source>Token Length</source>
        <translation type="vanished">Длина токена</translation>
    </message>
    <message>
        <source>%1 characters</source>
        <translation type="vanished">%1 символов</translation>
    </message>
    <message>
        <source>Wire Overhead</source>
        <translation type="vanished">Накладные расходы канала</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="110"/>
        <source>Failed to finalize output file:

%1</source>
        <translation>Не удалось завершить создание выходного файла:

%1</translation>
    </message>
    <message>
        <source>The operation succeeded, but GpgFrontend failed to finalize one or more output files.

Temporary output files have been cleaned up. Original files were kept unchanged.</source>
        <translation type="vanished">Операция прошла успешно, но GpgFrontend не удалось завершить создание одного или нескольких выходных файлов.

Временные выходные файлы были удалены. Исходные файлы остались без изменений.</translation>
    </message>
    <message>
        <source>The operation succeeded, but GpgFrontend failed to finalize one or more output files.

These output files were already written before the failure:
%1

The remaining temporary output files have been cleaned up.</source>
        <translation type="vanished">Операция выполнена, но GpgFrontend не удалось завершить запись одного или нескольких выходных файлов.

Эти выходные файлы уже были записаны до сбоя:
%1

Остальные временные выходные файлы удалены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="252"/>
        <source>Cannot read from the following files:

%1</source>
        <translation>Невозможно прочитать следующие файлы:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="265"/>
        <source>The target file &quot;%1&quot; already exists.

It will only be replaced after the operation succeeds.
Do you want to continue?</source>
        <translation>Целевой файл &quot;%1&quot; уже существует.

Он будет заменен только после успешного завершения операции.
Вы хотите продолжить?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="291"/>
        <source>Cannot write to the following files:

%1</source>
        <translation>Невозможно записать в следующие файлы:

%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="318"/>
        <source>For example the Following Key:</source>
        <translation>Например, следующий ключ:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="340"/>
        <source>Cannot Encrypt</source>
        <translation>Невозможно зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="360"/>
        <source>Confirm Recipients</source>
        <translation>Подтвердить получателей</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="363"/>
        <source>Encrypt to %n recipients?</source>
        <translation>
            <numerusform>Зашифровать для %n получателя?
Зашифровать для %n получателей?
Зашифровать для %n получателей?</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="364"/>
        <source>Each recipient will be able to see the key IDs of all the others.</source>
        <translation>Каждый получатель сможет видеть идентификаторы ключей всех остальных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="386"/>
        <source>Mixed Recipient Types</source>
        <translation>Смешанные типы получателей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="388"/>
        <source>Some recipients use post-quantum encryption and others do not.</source>
        <translation>Одни получатели используют постквантовое шифрование, а другие нет.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="396"/>
        <source>Encrypt Anyway</source>
        <translation>Всё равно зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="409"/>
        <source>Key Databases Changed</source>
        <translation>Базы данных ключей изменены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="410"/>
        <source>The key databases changed while this operation was being prepared, so it was not started. Please check the selected keys and try again.</source>
        <translation>Базы данных ключей изменились во время подготовки этой операции, поэтому она не была запущена. Проверьте выбранные ключи и повторите попытку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="504"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="730"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="946"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="493"/>
        <source>Encrypting</source>
        <translation>Шифрование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="580"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="589"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="968"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="428"/>
        <source>Decrypting</source>
        <translation>Расшифровка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="787"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="668"/>
        <source>The current tab has no text document to receive the result.</source>
        <translation>В текущей вкладке нет текстового документа для получения результата.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="305"/>
        <source>No Key Checked</source>
        <translation>Ключ не отмечен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="306"/>
        <source>Please check the key in the key toolbox on the right.</source>
        <translation>Отметьте ключ в панели ключей справа.</translation>
    </message>
    <message>
        <source>No Message Book Phrase Set</source>
        <translation type="vanished">Фраза книги сообщений не задана</translation>
    </message>
    <message>
        <source>You have not set a Message Book phrase.</source>
        <translation type="vanished">Вы не задали фразу для книги сообщений.</translation>
    </message>
    <message>
        <source>Instant messages are hidden using a shared &quot;Message Book&quot;. Without a phrase, GpgFrontend falls back to the built-in default book and that book ships in every copy of the program. It hides the format from a simple scanner, but anyone who knows GpgFrontend can still recognise your message for what it is.

Your message is OpenPGP-encrypted either way; what is at stake here is only whether it is recognisable as an encrypted message at all.

To get that, set a phrase and share it privately with the person you are writing to. You must both use exactly the same one.</source>
        <translation type="vanished">Мгновенные сообщения скрываются с помощью общей «Книги сообщений». Без фразы GpgFrontend использует встроенную книгу по умолчанию, которая поставляется в каждой копии программы. Она скрывает формат от простого сканера, но любой, кто знает GpgFrontend, всё равно сможет распознать ваше сообщение.

Ваше сообщение в любом случае зашифровано OpenPGP; под вопросом только то, будет ли оно вообще распознано как зашифрованное.

Чтобы этого избежать, задайте фразу и поделитесь ею лично с получателем. Вы оба должны использовать одну и ту же фразу.</translation>
    </message>
    <message>
        <source>Open Settings…</source>
        <translation type="vanished">Открыть настройки…</translation>
    </message>
    <message>
        <source>Continue with Default</source>
        <translation type="vanished">Продолжить с настройками по умолчанию</translation>
    </message>
    <message>
        <source>Continue, Don&apos;t Ask Again</source>
        <translation type="vanished">Продолжить, больше не спрашивать</translation>
    </message>
    <message>
        <source>Message Too Long</source>
        <translation type="vanished">Сообщение слишком длинное</translation>
    </message>
    <message>
        <source>This message is too long to send as an instant message.

The encrypted message is %1 bytes, and the instant-messaging format carries at most %2. Shorten the text, or send it as a normal OpenPGP message instead.</source>
        <translation type="vanished">Это сообщение слишком длинное для отправки как мгновенное сообщение.

Зашифрованное сообщение имеет размер %1 байт, а формат мгновенных сообщений поддерживает не более %2. Сократите текст или отправьте его как обычное сообщение OpenPGP.</translation>
    </message>
    <message>
        <source>Failed to prepare the instant message: the encrypted message could not be converted into a token.</source>
        <translation type="vanished">Не удалось подготовить мгновенное сообщение: зашифрованное сообщение не удалось преобразовать в токен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="747"/>
        <source>Encrypt Sign</source>
        <translation>Зашифровать и подписать</translation>
    </message>
    <message>
        <source>Message encrypted and signed for instant messaging.</source>
        <translation type="vanished">Сообщение зашифровано и подписано для мгновенной отправки.</translation>
    </message>
    <message>
        <source>Message encrypted for instant messaging.</source>
        <translation type="vanished">Сообщение зашифровано для мгновенной отправки.</translation>
    </message>
    <message>
        <source>An Instant Messaging section followed by the OpenPGP result.</source>
        <translation type="vanished">Раздел мгновенных сообщений, за которым следует результат OpenPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="817"/>
        <source>Checking Message</source>
        <translation>Проверка сообщения</translation>
    </message>
    <message>
        <source>Preparing Instant Message</source>
        <translation type="vanished">Подготовка мгновенного сообщения</translation>
    </message>
    <message>
        <source>Failed to decrypt instant message.</source>
        <translation type="vanished">Не удалось расшифровать мгновенное сообщение.</translation>
    </message>
    <message>
        <source>Instant message decrypted (not signed).</source>
        <translation type="vanished">Мгновенное сообщение расшифровано (не подписано).</translation>
    </message>
    <message>
        <source>Instant message decrypted.</source>
        <translation type="vanished">Мгновенное сообщение расшифровано.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="983"/>
        <source>The selected key contains a key that does not actually have a sign usage.</source>
        <translation>Выбранный ключ содержит ключ, не имеющий возможности подписи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="558"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1012"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="528"/>
        <source>Signing</source>
        <translation>Подписание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="391"/>
        <source>The message key is encrypted to every recipient, so anyone able to break the classical recipients&apos; encryption can read the whole message: the post-quantum protection does not hold for it. To keep that protection, encrypt separately for the post-quantum recipients.</source>
        <translation>Ключ сообщения шифруется для каждого получателя, поэтому тот, кто сможет взломать шифрование классических получателей, прочитает всё сообщение: постквантовая защита на него не распространяется. Чтобы сохранить эту защиту, зашифруйте сообщение отдельно для постквантовых получателей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="658"/>
        <source>Decrypt Verify</source>
        <translation>Расшифровать и проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="773"/>
        <source>The encoder did not return text.</source>
        <translation>Кодировщик не вернул текст.</translation>
    </message>
    <message>
        <source>Message encrypted, signed and encoded.</source>
        <translation type="vanished">Сообщение зашифровано, подписано и закодировано.</translation>
    </message>
    <message>
        <source>Message encrypted and encoded.</source>
        <translation type="vanished">Сообщение зашифровано и закодировано.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="835"/>
        <source>The message could not be decoded.</source>
        <translation>Не удалось декодировать сообщение.</translation>
    </message>
    <message>
        <source>Failed to decrypt the decoded message.</source>
        <translation type="vanished">Не удалось расшифровать декодированное сообщение.</translation>
    </message>
    <message>
        <source>Decoded message decrypted (not signed).</source>
        <translation type="vanished">Декодированное сообщение расшифровано (не подписано).</translation>
    </message>
    <message>
        <source>Decoded message decrypted.</source>
        <translation type="vanished">Декодированное сообщение расшифровано.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1038"/>
        <source>File to be Verified</source>
        <translation>Файл для проверки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1039"/>
        <source>Please provide An ABSOLUTE Path 
If Data And Signature is COMBINED within a single file, KEEP THIS EMPTY: </source>
        <translation>Укажите АБСОЛЮТНЫЙ путь 
Если данные и подпись ОБЪЕДИНЕНЫ в одном файле, ОСТАВЬТЕ ПОЛЕ ПУСТЫМ: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="603"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1065"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="455"/>
        <source>Verifying</source>
        <translation>Проверка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="648"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="730"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1104"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="584"/>
        <source>Encrypting and Signing</source>
        <translation>Шифрование и подписание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="667"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="680"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="1127"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="603"/>
        <source>Decrypting and Verifying</source>
        <translation>Расшифровка и проверка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="276"/>
        <source>Missing Keys</source>
        <translation>Отсутствующие ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="277"/>
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
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="297"/>
        <source>Verification Incomplete</source>
        <translation>Проверка не завершена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="298"/>
        <source>Verification was incomplete due to missing keys. You can manually import the keys later.</source>
        <translation>Проверка не завершена из-за отсутствия ключей. Вы можете импортировать ключи позже вручную.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="152"/>
        <source>Localize</source>
        <translation>Локализация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="155"/>
        <source>UTC</source>
        <translation>UTC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="166"/>
        <source>Local Time</source>
        <translation>Местное время</translation>
    </message>
    <message>
        <source>OK</source>
        <translation type="vanished">ОК</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="883"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="310"/>
        <source>No operation result available.</source>
        <translation>Результат операции недоступен.</translation>
    </message>
    <message>
        <source>FAILED</source>
        <translation type="vanished">ОШИБКА</translation>
    </message>
    <message>
        <source>WARNING</source>
        <translation type="vanished">ПРЕДУПРЕЖДЕНИЕ</translation>
    </message>
    <message>
        <source>Object: %1</source>
        <translation type="vanished">Объект: %1</translation>
    </message>
    <message>
        <source>Summary</source>
        <translation type="vanished">Сводка</translation>
    </message>
    <message>
        <source>Total operations: %1</source>
        <translation type="vanished">Всего операций: %1</translation>
    </message>
    <message>
        <source>Failed objects: %1</source>
        <translation type="vanished">Неудачных объектов: %1</translation>
    </message>
    <message>
        <source>Warning objects: %1</source>
        <translation type="vanished">Объектов с предупреждениями: %1</translation>
    </message>
    <message>
        <source>Details</source>
        <translation type="vanished">Подробности</translation>
    </message>
    <message>
        <source>Email Operation Failed</source>
        <translation type="vanished">Ошибка операции электронной почты</translation>
    </message>
    <message>
        <source>The operation could not be completed. See the reason and suggested solutions below.</source>
        <translation type="vanished">Не удалось выполнить операцию. См. причину и предлагаемые решения ниже.</translation>
    </message>
    <message>
        <source>Report</source>
        <translation type="vanished">Отчёт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="374"/>
        <source>Unsupported Operation</source>
        <translation>Неподдерживаемая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="419"/>
        <source>The decrypt operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция расшифровки для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="441"/>
        <source>The verify operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция проверки для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="473"/>
        <source>The encryption operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция шифрования для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="479"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="510"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="545"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="567"/>
        <source>No Key Selected</source>
        <translation>Ключ не выбран</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="480"/>
        <source>Please select a key for encryption.</source>
        <translation>Выберите ключ для шифрования.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="502"/>
        <source>The sign operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция подписи для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="511"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="568"/>
        <source>Please select a key for signing this document.</source>
        <translation>Выберите ключ для подписи этого документа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="517"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="574"/>
        <source>Please select only one key to sign this document.</source>
        <translation>Выберите только один ключ для подписи этого документа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="537"/>
        <source>The encrypt and sign operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция шифрования и подписи для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="546"/>
        <source>Please select a key for encrypting this document.</source>
        <translation>Выберите ключ для шифрования этого документа.</translation>
    </message>
    <message>
        <source>Please select a key for encrypt the EML.</source>
        <translation type="vanished">Выберите ключ для шифрования EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="595"/>
        <source>The decrypt and verify operation for the tab type &apos;%1&apos; is not supported.</source>
        <translation>Операция расшифровки и проверки для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <source>Please select a key for signing the EML.</source>
        <translation type="vanished">Выберите ключ для подписи EML.</translation>
    </message>
    <message>
        <source>Successful: %1</source>
        <translation type="vanished">Успешно: %1</translation>
    </message>
    <message>
        <source>Warnings: %1</source>
        <translation type="vanished">Предупреждений: %1</translation>
    </message>
    <message>
        <source>Failures: %1</source>
        <translation type="vanished">Неудач: %1</translation>
    </message>
    <message>
        <source>Operation Failed</source>
        <translation type="vanished">Операция не выполнена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="516"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotFunction.cpp" line="573"/>
        <source>Multiple Keys Selected</source>
        <translation>Выбрано несколько ключей</translation>
    </message>
    <message>
        <source>Please select only one key to sign the EML.</source>
        <translation type="vanished">Выберите только один ключ для подписи EML.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="336"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="91"/>
        <source>Save File</source>
        <translation>Сохранить файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="337"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="92"/>
        <source>Save the current File</source>
        <translation>Сохранить текущий файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="340"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="97"/>
        <source>Save As</source>
        <translation>Сохранить как</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="341"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="98"/>
        <source>Save the current File as...</source>
        <translation>Сохранить текущий файл как...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="103"/>
        <source>Print</source>
        <translation>Печать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="104"/>
        <source>Print Document</source>
        <translation>Печать документа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="137"/>
        <source>New Temporary Profile</source>
        <translation>Новый временный профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="138"/>
        <source>Start an empty, throwaway profile in a new window. Everything in it is destroyed when that window closes.</source>
        <translation>Запустить пустой одноразовый профиль в новом окне. Всё в нём уничтожается при закрытии этого окна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="175"/>
        <source>Quit</source>
        <translation>Выход</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="176"/>
        <source>Quit Program</source>
        <translation>Выход из программы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="180"/>
        <source>Undo</source>
        <translation>Отменить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="181"/>
        <source>Undo Last Edit Action</source>
        <translation>Отменить последнее действие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="184"/>
        <source>Redo</source>
        <translation>Повторить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="185"/>
        <source>Redo Last Edit Action</source>
        <translation>Повторить последнее действие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="188"/>
        <source>Zoom In</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="189"/>
        <source>Zoom in</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="193"/>
        <source>Zoom Out</source>
        <translation>Уменьшить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="194"/>
        <source>Zoom out</source>
        <translation>Уменьшить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="198"/>
        <source>Paste</source>
        <translation>Вставить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="199"/>
        <source>Paste Text From Clipboard</source>
        <translation>Вставить текст из буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="203"/>
        <source>Cut</source>
        <translation>Вырезать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="204"/>
        <source>Cut the current selection&apos;s contents to the clipboard</source>
        <translation>Вырезать содержимое выделенного в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="209"/>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="210"/>
        <source>Copy the current selection&apos;s contents to the clipboard</source>
        <translation>Копировать содержимое выделенного в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="214"/>
        <source>Quote</source>
        <translation>Цитировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="215"/>
        <source>Quote whole text</source>
        <translation>Цитировать весь текст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="219"/>
        <source>Select All</source>
        <translation>Выделить всё</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="220"/>
        <source>Select the whole text</source>
        <translation>Выделить весь текст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="224"/>
        <source>Find</source>
        <translation>Найти</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="225"/>
        <source>Find a word</source>
        <translation>Найти слово</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="229"/>
        <source>Remove spacing</source>
        <translation>Удалить пробелы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="231"/>
        <source>Remove double linebreaks, e.g. in pasted text from Web Mailer</source>
        <translation>Удалить двойные переносы строк, например, в тексте из веб-почты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="390"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="236"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="391"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="237"/>
        <source>Open settings dialog</source>
        <translation>Открыть диалог настроек</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="350"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="369"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="747"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="246"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="351"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="247"/>
        <source>Encrypt Message</source>
        <translation>Зашифровать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="368"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="254"/>
        <source>Encrypt and Sign Message</source>
        <translation>Зашифровать и подписать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="354"/>
        <location filename="../../../../src/ui/main_window/MainWindowGpgOperaFunction.cpp" line="571"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="260"/>
        <source>Decrypt</source>
        <translation>Расшифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="355"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="261"/>
        <source>Decrypt Message</source>
        <translation>Расшифровать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="374"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="268"/>
        <source>Decrypt and Verify Message</source>
        <translation>Расшифровать и проверить сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="358"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="274"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="328"/>
        <source>Crypto</source>
        <translation>Криптография</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="359"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="275"/>
        <source>Sign Message</source>
        <translation>Подписать сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="362"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="280"/>
        <source>Verify</source>
        <translation>Проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="363"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="281"/>
        <source>Verify Message</source>
        <translation>Проверить сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="379"/>
        <source>Encrypt and Encode</source>
        <translation>Зашифровать и закодировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="380"/>
        <source>Encrypt the message, then encode it with a module&apos;s encoder</source>
        <translation>Зашифруйте сообщение, затем закодируйте его с помощью кодировщика модуля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="395"/>
        <source>Message</source>
        <translation>Сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="287"/>
        <source>Sym. Encrypt</source>
        <translation>Симм. шифрование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="288"/>
        <source>Encrypt Message (Symmetric)</source>
        <translation>Зашифровать сообщение (симметрично)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="298"/>
        <source>New Keypair</source>
        <translation>Новая пара ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="299"/>
        <source>Generate KeyPair</source>
        <translation>Сгенерировать пару ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="306"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="441"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="595"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="308"/>
        <source>Import New Key From File</source>
        <translation>Импортировать новый ключ из файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="314"/>
        <source>Clipboard</source>
        <translation>Буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="316"/>
        <source>Import New Key From Clipboard</source>
        <translation>Импортировать новый ключ из буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="322"/>
        <source>Editor</source>
        <translation>Редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="323"/>
        <source>Import New Key From Editor</source>
        <translation>Импортировать новый ключ из редактора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="387"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="328"/>
        <source>Manage Keys</source>
        <translation>Управление ключами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="388"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="329"/>
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
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="341"/>
        <source>Open Smart Card Controller</source>
        <translation>Открыть управление смарт-картами</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="342"/>
        <source>Open Smart Card Controller Dialog</source>
        <translation>Открыть диалог управления смарт-картами</translation>
    </message>
    <message>
        <source>IM Encrypt</source>
        <translation type="vanished">IM-шифрование</translation>
    </message>
    <message>
        <source>Encrypt the current text into a compact, single-line format that is safe to paste into instant messengers. Recipients decrypt it with the normal Decrypt action.</source>
        <translation type="vanished">Зашифровать текущий текст в компактный однострочный формат, безопасный для вставки в мессенджеры. Получатели расшифровывают его обычным действием «Расшифровать».</translation>
    </message>
    <message>
        <source>IM Encrypt &amp;&amp; Sign</source>
        <translation type="vanished">IM-шифрование &amp;&amp; подписание</translation>
    </message>
    <message>
        <source>Encrypt and sign the current text into a compact, single-line format that is safe to paste into instant messengers. Recipients decrypt and verify it with the normal Decrypt &amp; Verify action.</source>
        <translation type="vanished">Зашифровать и подписать текущий текст в компактный однострочный формат, безопасный для вставки в мессенджеры. Получатели расшифровывают и проверяют его обычным действием «Расшифровать и проверить».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="334"/>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="393"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="443"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="603"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="448"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="610"/>
        <source>Workspace</source>
        <translation>Рабочая область</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="525"/>
        <source>Profiles</source>
        <translation>Профили</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="530"/>
        <source>Open Recent</source>
        <translation>Открыть недавние</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="549"/>
        <source>Advanced</source>
        <translation>Расширенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="603"/>
        <source>Open a file or directory</source>
        <translation>Открыть файл или каталог</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="611"/>
        <source>Open a text editor or file panel</source>
        <translation>Открыть текстовый редактор или панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="633"/>
        <source>Import</source>
        <translation>Импорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="634"/>
        <source>Import a key from file, editor, or clipboard</source>
        <translation>Импортировать ключ из файла, редактора или буфера обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="652"/>
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
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="799"/>
        <source>Status Panel</source>
        <translation>Панель состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="155"/>
        <source>Open Profile File</source>
        <translation>Открыть файл профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="192"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="252"/>
        <source>Cannot Open Profile</source>
        <translation>Не удаётся открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="230"/>
        <source>Nothing opened yet</source>
        <translation>Ещё ничего не открыто</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="243"/>
        <source>%1  (open in another window)</source>
        <translation>%1 (открыт в другом окне)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="321"/>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="340"/>
        <source>Cannot Export Profile</source>
        <translation>Не удаётся экспортировать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="322"/>
        <source>The application key is not available, so the profile could not be packed.</source>
        <translation>Ключ приложения недоступен, поэтому профиль не может быть упакован.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="330"/>
        <source>Exporting Profile</source>
        <translation>Экспорт профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="349"/>
        <source>&quot;%1&quot; was saved as an encrypted backup.</source>
        <translation>&quot;%1&quot; сохранён как зашифрованная резервная копия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="364"/>
        <source>Keep your passphrase safe. Without it, this file cannot be opened, and it cannot be recovered.</source>
        <translation>Храните парольную фразу в надёжном месте. Без неё этот файл нельзя открыть и нельзя восстановить.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="368"/>
        <source>Show in Folder</source>
        <translation>Показать в папке</translation>
    </message>
    <message>
        <source>&quot;%1&quot; was written to a single file.</source>
        <translation type="vanished">&quot;%1&quot; был записан в один файл.</translation>
    </message>
    <message>
        <source>Left out</source>
        <translation type="vanished">Не включено</translation>
    </message>
    <message>
        <source>A profile file only carries the profile itself.</source>
        <translation type="vanished">Файл профиля содержит только сам профиль.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="366"/>
        <source>Done</source>
        <translation>Готово</translation>
    </message>
    <message>
        <source>These were not included, because a profile file only carries the profile itself: %1</source>
        <translation type="vanished">Они не были включены, так как файл профиля содержит только сам профиль: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowSlotUI.cpp" line="348"/>
        <source>Profile Exported</source>
        <translation>Профиль экспортирован</translation>
    </message>
    <message>
        <source>&quot;%1&quot; was written to:</source>
        <translation type="vanished">&quot;%1&quot; записан в:</translation>
    </message>
    <message>
        <source>It can only be opened with the passphrase you chose. There is no way to recover it.</source>
        <translation type="vanished">Его можно открыть только с выбранной вами парольной фразой. Восстановить её невозможно.</translation>
    </message>
    <message>
        <source>It is not protected: anyone who gets this file can read the keys inside it.</source>
        <translation type="vanished">Он не защищён: любой, кто получит этот файл, сможет прочитать ключи внутри него.</translation>
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
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="332"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="59"/>
        <source>New Text Editor</source>
        <translation>Новый текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="74"/>
        <source>New File Panel</source>
        <translation>Новая панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="80"/>
        <source>File...</source>
        <translation>Файл...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="81"/>
        <source>Open a file in the file panel</source>
        <translation>Открыть файл на панели файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="86"/>
        <source>Directory...</source>
        <translation>Директория...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="87"/>
        <source>Open a directory in the file panel</source>
        <translation>Открыть директорию на панели файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="344"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="108"/>
        <source>Close Tab</source>
        <translation>Закрыть вкладку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="345"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="109"/>
        <source>Close the current tab</source>
        <translation>Закрыть текущую вкладку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="121"/>
        <source>Manage Profiles...</source>
        <translation>Управление профилями…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="122"/>
        <source>See every profile on this computer, and open, rename or remove one</source>
        <translation>Просмотреть все профили на этом компьютере, а также открыть, переименовать или удалить один из них.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="130"/>
        <source>New Profile...</source>
        <translation>Новый профиль…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="131"/>
        <source>Start an empty profile on this computer, with its own settings and keys</source>
        <translation>Создать пустой профиль на этом компьютере с собственными настройками и ключами.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="149"/>
        <source>Open Profile File...</source>
        <translation>Открыть файл профиля…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="150"/>
        <source>Work inside a profile file, leaving it a file. Nothing is added to this computer, and your changes go back into the same file.</source>
        <translation>Работать внутри файла профиля, оставляя его файлом. Ничего не добавляется на этот компьютер, а ваши изменения сохраняются обратно в тот же файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="161"/>
        <source>Import Profile File...</source>
        <translation>Импортировать файл профиля…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="162"/>
        <source>Copy a profile file into a new profile kept on this computer. The file is not used again afterwards.</source>
        <translation>Скопировать файл профиля в новый профиль на этом компьютере. После этого файл больше не используется.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="168"/>
        <source>Export This Profile...</source>
        <translation>Экспортировать этот профиль...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="169"/>
        <source>Write the profile this window is using out to a new profile file, to carry elsewhere or keep as a backup</source>
        <translation>Записать профиль, который использует это окно, в новый файл профиля — чтобы перенести его в другое место или сохранить как резервную копию.</translation>
    </message>
    <message>
        <source>Right-to-Left</source>
        <translation type="vanished">Справа налево</translation>
    </message>
    <message>
        <source>Lay this tab&apos;s text out from right to left, the way Arabic, Hebrew and Persian read</source>
        <translation type="vanished">Располагать текст этой вкладки справа налево, как читают на арабском, иврите и персидском</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="367"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="253"/>
        <source>Encrypt &amp;&amp; Sign</source>
        <translation>Зашифровать &amp;&amp; Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="373"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="267"/>
        <source>Decrypt &amp;&amp; Verify</source>
        <translation>Расшифровать &amp;&amp; Проверить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="335"/>
        <source>Open Module Controller</source>
        <translation>Открыть контроллер модулей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="336"/>
        <source>Open Module Controller Dialog</source>
        <translation>Открыть диалог контроллера модулей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="349"/>
        <source>About</source>
        <translation>О программе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="350"/>
        <source>Show the application&apos;s About box</source>
        <translation>Показать окно «О программе»</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="360"/>
        <source>Open Wizard</source>
        <translation>Открыть мастер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="361"/>
        <source>Open the wizard</source>
        <translation>Открыть мастер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="366"/>
        <source>Show Application Log</source>
        <translation>Показать журнал приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="367"/>
        <source>Show the application log view</source>
        <translation>Показать окно журнала приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="372"/>
        <source>Append Public Key to Editor</source>
        <translation>Добавить открытый ключ в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="373"/>
        <source>Append selected Keypair&apos;s Public Key to Editor</source>
        <translation>Добавить открытый ключ выбранной пары ключей в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="378"/>
        <source>Append Create DateTime to Editor</source>
        <translation>Добавить дату создания в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="379"/>
        <source>Append selected Key&apos;s creation date and time to Editor</source>
        <translation>Добавить дату и время создания выбранного ключа в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="384"/>
        <source>Append Expire DateTime to Editor</source>
        <translation>Добавить дату истечения в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="385"/>
        <source>Append selected Key&apos;s expiration date and time to Editor</source>
        <translation>Добавить дату и время истечения срока выбранного ключа в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="390"/>
        <source>Append Fingerprint to Editor</source>
        <translation>Добавить отпечаток в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="391"/>
        <source>Append selected Key&apos;s Fingerprint to Editor</source>
        <translation>Добавить отпечаток выбранного ключа в редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="396"/>
        <source>Copy Email</source>
        <translation>Копировать email</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="397"/>
        <source>Copy selected Keypair&apos;s to clipboard</source>
        <translation>Копировать выбранную пару ключей в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="402"/>
        <source>Copy Default UID</source>
        <translation>Копировать UID по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="403"/>
        <source>Copy selected Keypair&apos;s default UID to clipboard</source>
        <translation>Копировать UID по умолчанию выбранной пары ключей в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="408"/>
        <source>Copy Key ID</source>
        <translation>Копировать идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="409"/>
        <source>Copy selected Keypair&apos;s ID to clipboard</source>
        <translation>Копировать идентификатор выбранной пары ключей в буфер обмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="414"/>
        <source>Show Key Details</source>
        <translation>Показать сведения о ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="415"/>
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
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="420"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="421"/>
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
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="463"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="643"/>
        <source>Edit</source>
        <translation>Редактировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="494"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="625"/>
        <source>Keys</source>
        <translation>Ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowCommands.cpp" line="385"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="496"/>
        <source>Import Key</source>
        <translation>Импорт ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="564"/>
        <source>View</source>
        <translation>Вид</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="574"/>
        <source>Help</source>
        <translation>Помощь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="481"/>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="620"/>
        <source>Operations</source>
        <translation>Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="60"/>
        <source>Open a new text editor</source>
        <translation>Открыть новый текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="75"/>
        <source>Open a new file panel</source>
        <translation>Открыть новую панель файлов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="718"/>
        <source>Ready</source>
        <translation>Готово</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="722"/>
        <source>Key ToolBox</source>
        <translation>Панель ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="747"/>
        <source>Default</source>
        <translation>По умолчанию</translation>
    </message>
    <message>
        <source>Favourite</source>
        <translation type="vanished">Избранное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="755"/>
        <source>Key Group</source>
        <translation>Группа ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="762"/>
        <source>Only Public Key</source>
        <translation>Только открытый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/main_window/MainWindowUI.cpp" line="770"/>
        <source>Has Private Key</source>
        <translation>Имеет закрытый ключ</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleCommandsPanel</name>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="365"/>
        <source>Clear Log</source>
        <translation>Очистить журнал</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="344"/>
        <source>Filter commands...</source>
        <translation>Фильтровать команды...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="353"/>
        <source>Arguments as one JSON object; empty for none. Blobs cannot be given here.</source>
        <translation>Аргументы в виде одного JSON-объекта; пусто, если их нет. Блобы здесь указывать нельзя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="357"/>
        <source>Invoke as Host</source>
        <translation>Вызвать как хост</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="359"/>
        <source>Runs the command with the Host&apos;s own authority, which no module has: capability checks do not apply.</source>
        <translation>Выполняет команду с собственными полномочиями хоста, которых нет ни у одного модуля: проверки разрешений не применяются.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="446"/>
        <source>Arguments refused, %1</source>
        <translation>Аргументы отклонены, %1</translation>
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
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="124"/>
        <source>Show Modules Folder</source>
        <translation>Показать каталог модулей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="92"/>
        <source>Registered Modules</source>
        <translation>Зарегистрированные модули</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="93"/>
        <source>Global Register Table</source>
        <translation>Глобальная таблица регистрации</translation>
    </message>
    <message>
        <source>Debugger</source>
        <translation type="vanished">Отладчик</translation>
    </message>
    <message>
        <source>Tips: Module name front with &quot;*&quot; stands for integrated module.</source>
        <translation type="vanished">Подсказка: имя модуля с «*» означает встроенный модуль.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="90"/>
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
        <source>ID</source>
        <translation type="vanished">Идентификатор</translation>
    </message>
    <message>
        <source>Version</source>
        <translation type="vanished">Версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="101"/>
        <source>Search modules...</source>
        <translation>Поиск модулей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="102"/>
        <source>All</source>
        <translation>Все</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="106"/>
        <source>Inactive</source>
        <translation>Неактивные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="108"/>
        <source>Integrated</source>
        <translation>Встроенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="110"/>
        <source>External</source>
        <translation>Внешние</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="112"/>
        <source>Not Loaded</source>
        <translation>Не загруженные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="116"/>
        <source>Select a module to see its details.</source>
        <translation>Выберите модуль для просмотра его сведений.</translation>
    </message>
    <message>
        <source>SDK Version</source>
        <translation type="vanished">Версия SDK</translation>
    </message>
    <message>
        <source>Qt ENV Version</source>
        <translation type="vanished">Версия Qt ENV</translation>
    </message>
    <message>
        <source>Hash</source>
        <translation type="vanished">Хеш</translation>
    </message>
    <message>
        <source>Path</source>
        <translation type="vanished">Путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="118"/>
        <source>Listening Events</source>
        <translation>Прослушиваемые события</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="120"/>
        <source>Activate on Start</source>
        <translation>Активировать при запуске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="122"/>
        <source>Activate this module automatically when GpgFrontend starts.</source>
        <translation>Автоматически активировать этот модуль при запуске GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="123"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="129"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="126"/>
        <source>Search keys and values...</source>
        <translation>Поиск ключей и значений...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="127"/>
        <source>Expand All</source>
        <translation>Развернуть всё</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="128"/>
        <source>Collapse All</source>
        <translation>Свернуть все</translation>
    </message>
    <message>
        <source>Trigger Event</source>
        <translation type="vanished">Активировать событие</translation>
    </message>
    <message>
        <source>Upsert GRT Value</source>
        <translation type="vanished">Обновить значение GRT</translation>
    </message>
    <message>
        <source>%1 modules · %2 active</source>
        <translation type="vanished">Модули: %1 · Активно: %2</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="138"/>
        <source>%n module(s)</source>
        <translation>
            <numerusform>%n модуль
%n модуля
%n модулей</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="140"/>
        <source>%n active</source>
        <translation>
            <numerusform>Активно: %n</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <source>Please provide an Event ID</source>
        <translation type="vanished">Укажите идентификатор события</translation>
    </message>
    <message>
        <source>Namespace</source>
        <translation type="vanished">Пространство имён</translation>
    </message>
    <message>
        <source>Key</source>
        <translation type="vanished">Ключ</translation>
    </message>
    <message>
        <source>Value</source>
        <translation type="vanished">Значение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="214"/>
        <source>Modules are disabled. Change Module Discovery in Settings &gt; General to use them.</source>
        <translation>Загрузка модулей отключена. Включите её в настройках &gt; Общие, чтобы использовать модули.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="219"/>
        <source>Only integrated modules are loaded. To also load modules you have added, change Module Discovery in Settings &gt; General.</source>
        <translation>Загружены только встроенные модули. Чтобы загрузить внешние модули из каталога mods, измените политику загрузки модулей в настройках &gt; Общие.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="243"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="392"/>
        <source>Trust This Publisher Key...</source>
        <translation>Доверять ключу издателя...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="244"/>
        <source>Enable This Module</source>
        <translation>Включить этот модуль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="263"/>
        <source>Trust This Publisher Key?</source>
        <translation>Довериться этому ключу издателя?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="264"/>
        <source>&lt;p&gt;Modules signed by this publisher key will be offered for you to enable, one at a time. Trusting it does not enable anything by itself.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Publisher key fingerprint&lt;/b&gt;&lt;br/&gt;&lt;code&gt;%1&lt;/code&gt;&lt;/p&gt;&lt;p&gt;The key is the publisher&apos;s identity. Any name or website a module shows is only its own claim. A module signed with a different key will ask you again.&lt;/p&gt;&lt;p&gt;Only continue if you obtained this fingerprint from the module&apos;s author through a channel you trust.&lt;/p&gt;</source>
        <translation>&lt;p&gt;Модули, подписанные ключом этого издателя, будут предлагаться вам для включения по одному. Доверие к нему само по себе ничего не включает.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Отпечаток ключа издателя&lt;/b&gt;&lt;br/&gt;&lt;code&gt;%1&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Ключ — это личность издателя. Любое имя или веб-сайт, указанные модулем, — лишь его собственное заявление. Модуль, подписанный другим ключом, запросит подтверждение снова.&lt;/p&gt;&lt;p&gt;Продолжайте, только если вы получили этот отпечаток от автора модуля по каналу, которому доверяете.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="277"/>
        <source>&lt;p&gt;A module you enable runs inside GpgFrontend and can do anything GpgFrontend can. The capabilities it lists are what it asked for, not a limit.&lt;/p&gt;</source>
        <translation>&lt;p&gt;Модуль, который вы включаете, работает внутри GpgFrontend и может делать всё, что умеет GpgFrontend. Перечисленные возможности — это то, что он запросил, а не ограничение.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="285"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="302"/>
        <source>Not Saved</source>
        <translation>Не сохранено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="286"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="303"/>
        <source>That decision could not be saved, so nothing has changed.</source>
        <translation>Не удалось сохранить это решение, поэтому ничего не изменилось.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="311"/>
        <source>Enabled</source>
        <translation>Включён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="312"/>
        <source>This module will be loaded the next time GpgFrontend starts.</source>
        <translation>Модуль будет загружен при следующем запуске GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="346"/>
        <source>Needs Approval</source>
        <translation>Требует одобрения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="346"/>
        <source>Refused</source>
        <translation>Отклонено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="354"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="355"/>
        <source>Waiting for you</source>
        <translation>Ожидает вашего решения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="356"/>
        <source>Not loaded</source>
        <translation>Не загружен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="359"/>
        <source>Identifier</source>
        <translation>Идентификатор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="361"/>
        <source>Descriptor</source>
        <translation>Дескриптор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="369"/>
        <source>Publisher key</source>
        <translation>Ключ издателя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="371"/>
        <source>You have trusted this publisher key.</source>
        <translation>Вы доверяете этому ключу издателя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="372"/>
        <source>You have not trusted this publisher key. Before trusting it, compare its fingerprint with the one the publisher published.</source>
        <translation>Вы не доверяете этому ключу издателя. Прежде чем довериться ему, сравните его отпечаток с тем, который опубликовал издатель.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="391"/>
        <source>Publisher Key Trusted</source>
        <translation>Доверенный ключ издателя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="445"/>
        <source>● Active</source>
        <translation>● Активный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="445"/>
        <source>○ Inactive</source>
        <translation>○ Неактивный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="449"/>
        <source>Auto Start</source>
        <translation>Автозапуск</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="454"/>
        <source>by %1</source>
        <translation>от %1</translation>
    </message>
    <message>
        <source>N/A</source>
        <translation type="vanished">Н/Д</translation>
    </message>
    <message>
        <source>N/A (integrated)</source>
        <translation type="vanished">Н/Д (встроенный)</translation>
    </message>
    <message>
        <source>Click to open the containing folder</source>
        <translation type="vanished">Нажмите, чтобы открыть папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="472"/>
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
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="104"/>
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
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="474"/>
        <source>Deactivate</source>
        <translation>Деактивировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="475"/>
        <source>Activate</source>
        <translation>Активировать</translation>
    </message>
    <message>
        <source>Disable Auto Activate</source>
        <translation type="vanished">Отключить автоактивацию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="95"/>
        <source>Events</source>
        <translation>События</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="99"/>
        <source>Commands</source>
        <translation>Команды</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleControllerDialog.cpp" line="97"/>
        <source>Module Status</source>
        <translation>Состояние модулей</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleEventsPanel</name>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="189"/>
        <source>One key=value per line</source>
        <translation>Одна пара ключ=значение в строке</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="192"/>
        <source>Fire</source>
        <translation>Отправить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="196"/>
        <source>Clear Log</source>
        <translation>Очистить журнал</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="199"/>
        <source>Event</source>
        <translation>Событие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="201"/>
        <source>Listeners</source>
        <translation>Слушатели</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="202"/>
        <source>Parameters</source>
        <translation>Параметры</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="235"/>
        <source>Not an event this build fires.</source>
        <translation>Это не событие, отправляемое этой сборкой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="247"/>
        <source>No active module listens to it.</source>
        <translation>Ни один активный модуль не прослушивает его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="256"/>
        <source>Parameters refused, %1</source>
        <translation>Параметры отклонены, %1</translation>
    </message>
    <message numerus="yes">
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="261"/>
        <source>Fired %1 to %n listener(s)</source>
        <translation>
            <numerusform>Событие %1 отправлено %n слушателю
Событие %1 отправлено %n слушателям
Событие %1 отправлено %n слушателям</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleItemDelegate</name>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="168"/>
        <source>Needs Approval</source>
        <translation>Требует одобрения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="168"/>
        <source>Refused</source>
        <translation>Отказано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="181"/>
        <source>Integrated</source>
        <translation>Встроенный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="181"/>
        <source>External</source>
        <translation>Внешний</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="196"/>
        <source>Signed</source>
        <translation>Подписано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="196"/>
        <source>Unsigned</source>
        <translation>Не подписано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/ModuleListView.cpp" line="203"/>
        <source>Auto</source>
        <translation>Авто</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ModuleStatusPanel</name>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="284"/>
        <source>Commands</source>
        <translation>Команды</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="283"/>
        <source>Module</source>
        <translation>Модуль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="283"/>
        <source>State</source>
        <translation>Состояние</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="283"/>
        <source>Origin</source>
        <translation>Источник</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="283"/>
        <source>Entry Gate</source>
        <translation>Входной шлюз</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="284"/>
        <source>Listening</source>
        <translation>Прослушивание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="284"/>
        <source>Answers Owed</source>
        <translation>Ожидается ответов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="284"/>
        <source>Calls</source>
        <translation>Вызовы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="285"/>
        <source>Widgets</source>
        <translation>Виджеты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="291"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="294"/>
        <source>Read-only. Activate and deactivate on the first tab.</source>
        <translation>Только для чтения. Активировать и деактивировать на первой вкладке.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="326"/>
        <source>Integrated</source>
        <translation>Интегрированный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="326"/>
        <source>External</source>
        <translation>Внешний</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="328"/>
        <source>Closed</source>
        <translation>Закрыто</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleDeveloperPanel.cpp" line="328"/>
        <source>Open</source>
        <translation>Открыто</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::MoveKeyToCardPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="126"/>
        <source>Select Key to Move to Card</source>
        <translation>Выберите ключ для перемещения на смарт-карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="131"/>
        <source>Choose a Key or Subkey to Move</source>
        <translation>Выберите ключ или подключ для перемещения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="137"/>
        <source>Select the single private key or subkey to move onto the smart card. Expand a key to choose a particular subkey. Only parts that can be stored on a card are selectable.</source>
        <translation>Выберите один закрытый ключ или подключ для перемещения на смарт-карту. Разверните ключ, чтобы выбрать конкретный подключ. Выбирать можно только те части, которые можно сохранить на смарт-карте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="151"/>
        <source>Database</source>
        <translation>База данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="188"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="191"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="196"/>
        <source>Select One Key</source>
        <translation>Выберите один ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/MoveKeyToCardPicker.cpp" line="197"/>
        <source>Please select exactly one key or subkey to move to the card.</source>
        <translation>Пожалуйста, выберите ровно один ключ или подключ для перемещения на смарт-карту.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::NetworkTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="66"/>
        <source>Proxy</source>
        <translation>Прокси</translation>
    </message>
    <message>
        <source>Network Ability</source>
        <translation type="vanished">Сетевые возможности</translation>
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
        <source>Automatically fetch key publish status from key server.</source>
        <translation type="vanished">Автоматически получать статус публикации ключа с сервера ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="133"/>
        <source>Test Server Url Accessibility</source>
        <translation>Проверить доступность URL сервера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="134"/>
        <source>Server Url</source>
        <translation>URL сервера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="144"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="145"/>
        <source>Successfully connect to the target server through the proxy server.</source>
        <translation>Успешное подключение к целевому серверу через прокси-сервер.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="149"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="150"/>
        <source>Unable to connect to the target server through the proxy server. Proxy settings may be invalid.</source>
        <translation>Не удалось подключиться к целевому серверу через прокси-сервер. Настройки прокси могут быть недопустимы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="160"/>
        <source>Test Proxy Server Connection...</source>
        <translation>Тест подключения к прокси-серверу...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsNetwork.cpp" line="161"/>
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
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="94"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="142"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="210"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="447"/>
        <source>Loading...</source>
        <translation>Загрузка...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="144"/>
        <source>Ln 1, Col 1 · 0 chars</source>
        <translation>Стр. 1, Стлб. 1 · 0 симв.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="145"/>
        <source>Number of characters in the editor.</source>
        <translation>Количество символов в редакторе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="147"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="238"/>
        <source>LF</source>
        <translation>LF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="148"/>
        <source>Line ending style.</source>
        <translation>Стиль окончания строк.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="152"/>
        <source>Text encoding.</source>
        <translation>Кодировка текста.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="161"/>
        <source>SHA-256 checksum of editor content.</source>
        <translation>Контрольная сумма SHA-256 содержимого редактора.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="232"/>
        <source>Ln %1, Col %2 · %3 chars%4</source>
        <translation>Стр. %1, Стлб. %2 · %3 симв.%4</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="238"/>
        <source>CRLF</source>
        <translation>CRLF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="244"/>
        <source>The document has unsaved changes.</source>
        <translation>В документе есть несохраненные изменения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="245"/>
        <source>The document is unchanged.</source>
        <translation>Документ не изменен.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="533"/>
        <source>Loading... %1 KB</source>
        <translation>Загрузка... %1 КБ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="895"/>
        <source>Message</source>
        <translation>Сообщение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="896"/>
        <source>Raw Source</source>
        <translation>Исходный текст</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="975"/>
        <source>Protected Document</source>
        <translation>Защищенный документ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="1001"/>
        <source>Stop Editing</source>
        <translation>Прекратить редактирование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="1002"/>
        <source>Edit Raw Source</source>
        <translation>Редактировать исходный текст</translation>
    </message>
    <message>
        <source>You are editing the raw source. What you type here is the document.</source>
        <translation type="vanished">Вы редактируете исходный текст. Все, что вы вводите здесь, является документом.</translation>
    </message>
    <message>
        <source>Read-only. Unlock to edit the source.</source>
        <translation type="vanished">Только чтение. Разблокируйте для редактирования исходного текста.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="1060"/>
        <source>Text Direction</source>
        <translation>Направление текста</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="1067"/>
        <source>Automatic</source>
        <translation>Автоматически</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="1068"/>
        <source>Left-to-Right</source>
        <translation>Слева направо</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="1069"/>
        <source>Right-to-Left</source>
        <translation>Справа налево</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="1083"/>
        <source>Which way the text runs. Automatic gives every line the direction of its own first letter.</source>
        <translation>Направление текста. Автоматический режим задает каждой строке направление по первой букве.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="151"/>
        <location filename="../../../../src/ui/widgets/PlainTextEditorPage.cpp" line="239"/>
        <source>UTF-8</source>
        <translation>UTF-8</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileCreateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="148"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="173"/>
        <source>New Profile</source>
        <translation>Новый профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="174"/>
        <source>Its own keys and settings, opened in a window of its own.</source>
        <translation>Собственные ключи и настройки, открываемые в отдельном окне.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="187"/>
        <source>Create Profile</source>
        <translation>Создать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="209"/>
        <source>for example: Work</source>
        <translation>например: Работа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="213"/>
        <source>Shown in the profile list and the status bar. You can change it later.</source>
        <translation>Отображается в списке профилей и в строке состояния. Вы можете изменить его позже.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="217"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="231"/>
        <source>Keys live inside this profile and can be exported with it as one file.</source>
        <translation>Ключи хранятся внутри этого профиля и могут быть экспортированы вместе с ним в виде одного файла.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="233"/>
        <source>Recommended</source>
        <translation>Рекомендуется</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="236"/>
        <source>Your existing keys, shared with the system. Not carried in a profile file.</source>
        <translation>Ваши существующие ключи, общие с системой. Не переносятся в файл профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="252"/>
        <source>Keys</source>
        <translation>Ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="264"/>
        <source>Start with this window&apos;s settings</source>
        <translation>Начать с настроек этого окна</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="331"/>
        <source>Never copied: keys, key databases, the workspace folder, key protection and proxy credentials.</source>
        <translation>Никогда не копируются: ключи, базы данных ключей, папка рабочей области, защита ключей и учётные данные прокси.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="338"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="230"/>
        <source>Start with an empty keyring</source>
        <translation>Начать с пустой связки ключей</translation>
    </message>
    <message>
        <source>Keys live inside this profile. It stays separate from everything else and can be exported as a single file. It opens with no keys, so you will import or create them here.</source>
        <translation type="vanished">Ключи хранятся внутри этого профиля. Он отделен от всего остального и может быть экспортирован в виде одного файла. Он открывается без ключей, поэтому вы будете импортировать или создавать их здесь.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileCreateDialog.cpp" line="235"/>
        <source>Use the system GnuPG keyring</source>
        <translation>Использовать системную связку ключей GnuPG</translation>
    </message>
    <message>
        <source>The same keys you already have. They are shared with the rest of the system, so they are not really separate and cannot be carried inside a profile file.</source>
        <translation type="vanished">Те же ключи, которые у вас уже есть. Они являются общими для всей системы, поэтому не являются по-настоящему отдельными и не могут быть перенесены в файл профиля.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileExportDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="53"/>
        <source>Export backup</source>
        <translation>Экспорт резервной копии</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="57"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="279"/>
        <source>Export Profile</source>
        <translation>Экспорт профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="88"/>
        <source>Check what the backup file will contain.</source>
        <translation>Проверьте, что будет содержать файл резервной копии.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="125"/>
        <source>Choose a passphrase to encrypt the backup. It cannot be recovered, so keep it somewhere safe.</source>
        <translation>Выберите парольную фразу для шифрования резервной копии. Её невозможно восстановить, поэтому храните её в надёжном месте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="182"/>
        <source>Check the summary, then choose where to save the file.</source>
        <translation>Проверьте сводку, затем выберите, где сохранить файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="198"/>
        <source>No location chosen yet</source>
        <translation>Место ещё не выбрано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="206"/>
        <source>Choose where to save the file</source>
        <translation>Выберите, где сохранить файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="210"/>
        <source>Browse...</source>
        <translation>Обзор...</translation>
    </message>
    <message>
        <source>&quot;%1&quot; is written into a single file you can copy to another computer and import there.</source>
        <translation type="vanished">&quot;%1&quot; записан в один файл, который можно скопировать на другой компьютер и импортировать туда.</translation>
    </message>
    <message>
        <source>— choose a file —</source>
        <translation type="vanished">— выберите файл —</translation>
    </message>
    <message>
        <source>No file chosen yet.</source>
        <translation type="vanished">Файл ещё не выбран.</translation>
    </message>
    <message>
        <source>Choose...</source>
        <translation type="vanished">Выбрать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportDialog.cpp" line="193"/>
        <source>Save to</source>
        <translation>Сохранить в</translation>
    </message>
    <message>
        <source>The file cannot be opened without this passphrase, and it cannot be recovered.</source>
        <translation type="vanished">Без этой парольной фразы файл невозможно открыть, и восстановить её нельзя.</translation>
    </message>
    <message>
        <source>Export &quot;%1&quot;</source>
        <translation type="vanished">Экспорт &quot;%1&quot;</translation>
    </message>
    <message>
        <source>Everything below is written into a single file you can copy to another computer and import there.</source>
        <translation type="vanished">Всё ниже записано в один файл, который можно скопировать на другой компьютер и импортировать туда.</translation>
    </message>
    <message>
        <source>Total</source>
        <translation type="vanished">Итого</translation>
    </message>
    <message>
        <source>What goes in</source>
        <translation type="vanished">Что включается</translation>
    </message>
    <message>
        <source>The file cannot be opened without this passphrase, and there is no way to recover it. This computer&apos;s keychain is not used: the file has to open on another computer.</source>
        <translation type="vanished">Файл нельзя открыть без этой парольной фразы, и восстановить её невозможно. Связка ключей этого компьютера не используется: файл должен открываться на другом компьютере.</translation>
    </message>
    <message>
        <source>XChaCha20-Poly1305, with a key derived from your passphrase using Argon2id.</source>
        <translation type="vanished">XChaCha20-Poly1305, с ключом, полученным из вашей парольной фразы с помощью Argon2id.</translation>
    </message>
    <message>
        <source>%1 free</source>
        <translation type="vanished">%1 свободно</translation>
    </message>
    <message>
        <source>Choose where to save the file to continue.</source>
        <translation type="vanished">Выберите, куда сохранить файл, чтобы продолжить.</translation>
    </message>
    <message>
        <source>Include my workspace files (%1)</source>
        <translation type="vanished">Включить мои рабочие файлы (%1)</translation>
    </message>
    <message>
        <source>Protection</source>
        <translation type="vanished">Защита</translation>
    </message>
    <message>
        <source>Protect with a passphrase</source>
        <translation type="vanished">Защитить парольной фразой</translation>
    </message>
    <message>
        <source>Passphrase</source>
        <translation type="vanished">Парольная фраза</translation>
    </message>
    <message>
        <source>Repeat</source>
        <translation type="vanished">Повторите</translation>
    </message>
    <message>
        <source>No protection</source>
        <translation type="vanished">Без защиты</translation>
    </message>
    <message>
        <source>Export</source>
        <translation type="vanished">Экспорт</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation type="vanished">Настройки</translation>
    </message>
    <message>
        <source>Saved state, key groups and categories</source>
        <translation type="vanished">Сохранённое состояние, группы ключей и категории</translation>
    </message>
    <message>
        <source>Keys stored inside this profile</source>
        <translation type="vanished">Ключи, хранящиеся внутри этого профиля</translation>
    </message>
    <message>
        <source>Logs and modules are never included. Keys kept outside this profile, such as the system GnuPG keyring, stay where they are.</source>
        <translation type="vanished">Журналы и модули никогда не включаются. Ключи, хранящиеся вне этого профиля, например системная связка ключей GnuPG, остаются на своих местах.</translation>
    </message>
    <message>
        <source>The file cannot be opened without this passphrase, and there is no way to recover it. Keys wrapped by this computer&apos;s keychain are not used: the file has to open on another computer.</source>
        <translation type="vanished">Файл невозможно открыть без этой парольной фразы, и восстановить её невозможно. Ключи, обёрнутые связкой ключей этого компьютера, не используются: файл должен быть открыт на другом компьютере.</translation>
    </message>
    <message>
        <source>The two entries do not match.</source>
        <translation type="vanished">Две записи не совпадают.</translation>
    </message>
    <message>
        <source>Anyone who gets this file can read your keys and everything in the profile, and can change it before you import it.</source>
        <translation type="vanished">Любой, кто получит этот файл, сможет прочитать ваши ключи и всё содержимое профиля, а также изменить его до того, как вы его импортируете.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::ProfileManagerDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="72"/>
        <source>Profiles On This Computer</source>
        <translation>Профили на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="89"/>
        <source>The profiles kept on this computer. Each one has its own settings, keys and saved state. Opening one starts a new window; this window stays exactly as it is.</source>
        <translation>Профили, хранящиеся на этом компьютере. Каждый имеет свои настройки, ключи и сохранённое состояние. Открытие профиля запускает новое окно; это окно остаётся в точности таким, как есть.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Folder</source>
        <translation>Папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Last Opened</source>
        <translation>Последнее открытие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="107"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="117"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="119"/>
        <source>New...</source>
        <translation>Создать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="120"/>
        <source>New Temporary</source>
        <translation>Создать временный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="121"/>
        <source>Import...</source>
        <translation>Импортировать...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="122"/>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="123"/>
        <source>Open Folder</source>
        <translation>Открыть папку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="125"/>
        <source>Open the selected profile in a new window</source>
        <translation>Открыть выбранный профиль в новом окне</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="126"/>
        <source>Start an empty profile on this computer</source>
        <translation>Создать пустой профиль на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="128"/>
        <source>Start an empty, throwaway profile in a new window. Everything in it is destroyed when that window closes.</source>
        <translation>Запустить пустой временный профиль в новом окне. Всё в нём уничтожается при закрытии этого окна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="134"/>
        <source>Copy a profile file into a new profile kept on this computer. The file is not used again afterwards.</source>
        <translation>Скопировать файл профиля в новый профиль на этом компьютере. Файл больше не используется.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="137"/>
        <source>Remove the selected profile and everything in it, permanently</source>
        <translation>Удалить выбранный профиль и всё его содержимое навсегда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="139"/>
        <source>Show the selected profile&apos;s folder in the file manager</source>
        <translation>Показать папку выбранного профиля в файловом менеджере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="149"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="186"/>
        <source>%1  (open)</source>
        <translation>%1  (открыт)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="206"/>
        <source>This window</source>
        <translation>Это окно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="208"/>
        <source>Not created yet</source>
        <translation>Ещё не создан</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="211"/>
        <source>Open in another window</source>
        <translation>Открыть в другом окне</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="250"/>
        <source>Cannot Open Profile</source>
        <translation>Не удаётся открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="266"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="275"/>
        <source>Delete Profile</source>
        <translation>Удалить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="267"/>
        <source>Delete &quot;%1&quot;?</source>
        <translation>Удалить &quot;%1&quot;?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="268"/>
        <source>Its keys, settings and saved state are removed from this computer permanently.</source>
        <translation>Его ключи, настройки и сохранённое состояние будут безвозвратно удалены с этого компьютера.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="276"/>
        <source>This cannot be undone. Anything stored only in &quot;%1&quot; will be lost.</source>
        <translation>Это действие нельзя отменить. Всё, что хранится только в &quot;%1&quot;, будет потеряно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="285"/>
        <source>Profile Is Open</source>
        <translation>Профиль открыт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="286"/>
        <source>&quot;%1&quot; is open in another window. Close it first.</source>
        <translation>&quot;%1&quot; открыт в другом окне. Сначала закройте его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="304"/>
        <source>Cannot Delete Profile</source>
        <translation>Невозможно удалить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileManagerDialog.cpp" line="305"/>
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
    <name>GpgFrontend::UI::RecipientConfirm</name>
    <message numerus="yes">
        <location filename="../../../../src/ui/main_window/RecipientConfirm.cpp" line="60"/>
        <source>Key group, %n key(s)</source>
        <translation>
            <numerusform>Группа ключей, %n ключ
Группа ключей, %n ключа
Группа ключей, %n ключей</numerusform>
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
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
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="716"/>
        <source>Engine Version:</source>
        <translation>Версия движка:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="718"/>
        <source>Rust Compiler:</source>
        <translation>Компилятор Rust:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="721"/>
        <source>Target:</source>
        <translation>Целевая платформа:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="725"/>
        <source>Build Profile:</source>
        <translation>Профиль сборки:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="730"/>
        <source>rPGP Engine</source>
        <translation>Движок rPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="737"/>
        <source>Key Dependencies</source>
        <translation>Основные зависимости</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="754"/>
        <source>Copy Engine Information</source>
        <translation>Скопировать сведения о движке</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::RpgpTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="43"/>
        <source>Password Cache</source>
        <translation>Кэш паролей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="46"/>
        <source>Password Cache TTL (minutes):</source>
        <translation>TTL кэша паролей (мин):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="48"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="55"/>
        <source> min</source>
        <translation>мин</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="50"/>
        <source>Idle time the rPGP engine keeps an entered passphrase cached. The window is renewed each time the passphrase is used.</source>
        <translation>Время бездействия, в течение которого движок rPGP хранит введённую парольную фразу в кэше. Окно обновляется каждый раз при использовании парольной фразы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="53"/>
        <source>Password Cache Max TTL (minutes):</source>
        <translation>Макс. TTL кэша паролей (мин):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="57"/>
        <source>Absolute lifetime of a cached passphrase, measured from when it was first entered, regardless of use. Never shorter than the TTL.</source>
        <translation>Абсолютное время жизни кэшированной парольной фразы, измеряемое с момента её первого ввода, независимо от использования. Не может быть короче TTL.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="61"/>
        <source>These options only apply to the rPGP engine&apos;s in-memory passphrase cache.</source>
        <translation>Эти параметры применяются только к кэшу парольных фраз в памяти движка rPGP.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="64"/>
        <source>Passphrase Prompt</source>
        <translation>Запрос парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="68"/>
        <source>Passphrase Prompt Timeout (seconds):</source>
        <translation>Тайм-аут запроса парольной фразы (сек):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="70"/>
        <source> s</source>
        <translation>с</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="71"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="73"/>
        <source>Time the passphrase prompt waits for an answer before it closes itself and cancels the operation. Set to 0 to let it wait indefinitely.</source>
        <translation>Время ожидания ответа на запрос парольной фразы, после которого он закрывается и отменяет операцию. Установите 0, чтобы ждать бесконечно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="78"/>
        <source>Applies to the passphrase prompt GpgFrontend shows for the rPGP engine. GnuPG keys are unlocked through pinentry, which has its own timeout.</source>
        <translation>Применяется к запросу парольной фразы, который GpgFrontend показывает для движка rPGP. Ключи GnuPG разблокируются через pinentry, у которого свой тайм-аут.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="82"/>
        <source>Passphrase Encryption</source>
        <translation>Шифрование с парольной фразой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="87"/>
        <source>Argon2 Parameters:</source>
        <translation>Параметры Argon2:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="88"/>
        <source>High memory, 2 GiB</source>
        <translation>Большой объём памяти, 2 ГиБ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="90"/>
        <source>Low memory, 64 MiB</source>
        <translation>Малый объём памяти, 64 МиБ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="93"/>
        <source>How much work it takes to turn the passphrase into an encryption key. More memory makes a stolen message far more expensive to crack.</source>
        <translation>Это определяет, сколько работы требуется для преобразования парольной фразы в ключ шифрования. Больший объём памяти делает взлом украденного сообщения гораздо более дорогим.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsRpgp.cpp" line="98"/>
        <source>Decrypting a message needs as much memory as encrypting it, on every machine that opens it. Choose the low-memory option if this computer or the recipient&apos;s cannot spare 2 GiB. This applies only to messages encrypted from now on. </source>
        <translation>Для расшифровки сообщения требуется столько же памяти, сколько и для его шифрования, на каждой машине, которая его открывает. Выберите вариант с малым объёмом памяти, если этот компьютер или компьютер получателя не может выделить 2 ГиБ. Это относится только к сообщениям, зашифрованным с этого момента.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SecretEntryPanel</name>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="65"/>
        <source>Generate</source>
        <translation>Сгенерировать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="68"/>
        <source>Invent a strong passphrase and show it, so you can write it down</source>
        <translation>Придумайте надёжную парольную фразу и покажите её, чтобы вы могли её записать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="220"/>
        <source>Generate a strong passphrase</source>
        <translation>Сгенерировать надёжную парольную фразу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="389"/>
        <source>A passphrase could not be generated.</source>
        <translation>Не удалось сгенерировать парольную фразу.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/SecretEntryPanel.cpp" line="408"/>
        <source>Copied to the clipboard. Save it somewhere safe now — it is not stored anywhere, and it cannot be recovered later.</source>
        <translation>Скопировано в буфер обмена. Сохраните её в надёжном месте сейчас — она нигде не хранится, и её невозможно будет восстановить позже.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SetOwnerTrustLevel</name>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <source>Undefined</source>
        <translation>Не определено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <source>Marginal</source>
        <translation>Частичное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="47"/>
        <source>Full</source>
        <translation>Полное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="48"/>
        <source>Ultimate</source>
        <translation>Абсолютное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="73"/>
        <source>Modify Owner Trust Level</source>
        <translation>Изменить уровень доверия владельца</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="74"/>
        <source>Trust for the Key Pair:</source>
        <translation>Доверие для пары ключей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="84"/>
        <source>Failed</source>
        <translation>Не удалось</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SetOwnerTrustLevel.cpp" line="85"/>
        <source>Modify Owner Trust Level failed.</source>
        <translation>Не удалось изменить уровень доверия владельца.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SettingsDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="94"/>
        <source>General</source>
        <translation>Общие</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="95"/>
        <source>Appearance</source>
        <translation>Внешний вид</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="124"/>
        <source>Network</source>
        <translation>Сеть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="96"/>
        <source>Key Databases</source>
        <translation>Базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="87"/>
        <source>Application</source>
        <translation>Приложение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="88"/>
        <source>Keys &amp; Engines</source>
        <translation>Ключи и движки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="89"/>
        <source>Features</source>
        <translation>Функции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="90"/>
        <source>System</source>
        <translation>Система</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="97"/>
        <source>GnuPG</source>
        <translation>GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="98"/>
        <source>Advanced</source>
        <translation>Расширенные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="110"/>
        <source>startup</source>
        <translation>Запуск</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="110"/>
        <source>confirm import</source>
        <translation>Подтверждать импорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="110"/>
        <source>language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="110"/>
        <source>locale</source>
        <translation>Локаль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="111"/>
        <source>translation</source>
        <translation>Перевод</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="111"/>
        <source>data</source>
        <translation>Данные</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="111"/>
        <source>cache</source>
        <translation>Кэш</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>theme</source>
        <translation>Тема</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>icon</source>
        <translation>Значок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>font size</source>
        <translation>Размер шрифта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="115"/>
        <source>font family</source>
        <translation>Гарнитура шрифта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="116"/>
        <source>toolbar</source>
        <translation>Панель инструментов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="116"/>
        <source>actions</source>
        <translation>Действия</translation>
    </message>
    <message>
        <source>instant messaging</source>
        <translation type="vanished">Мгновенные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="116"/>
        <source>text editor</source>
        <translation>Текстовый редактор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="116"/>
        <source>status panel</source>
        <translation>Панель состояния</translation>
    </message>
    <message>
        <source>text direction</source>
        <translation type="vanished">направление текста</translation>
    </message>
    <message>
        <source>right-to-left</source>
        <translation type="vanished">справа налево</translation>
    </message>
    <message>
        <source>rtl</source>
        <translation type="vanished">справа налево</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="117"/>
        <source>arabic</source>
        <translation>арабский</translation>
    </message>
    <message>
        <source>hebrew</source>
        <translation type="vanished">иврит</translation>
    </message>
    <message>
        <source>persian</source>
        <translation type="vanished">персидский</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="126"/>
        <source>proxy</source>
        <translation>Прокси</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="126"/>
        <source>socks</source>
        <translation>SOCKS</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="126"/>
        <source>http</source>
        <translation>HTTP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="126"/>
        <source>timeout</source>
        <translation>тайм-аут</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="127"/>
        <source>connection</source>
        <translation>соединение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="133"/>
        <source>keyring</source>
        <translation>связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="133"/>
        <source>gpg home</source>
        <translation>домашний каталог GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="133"/>
        <source>database path</source>
        <translation>путь к базе данных</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="134"/>
        <source>built-in</source>
        <translation>встроенный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="134"/>
        <source>default</source>
        <translation>по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="141"/>
        <source>gpgme</source>
        <translation>gpgme</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="141"/>
        <source>gpgconf</source>
        <translation>gpgconf</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="141"/>
        <source>binary path</source>
        <translation>путь к исполняемому файлу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="141"/>
        <source>custom install</source>
        <translation>пользовательская установка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="142"/>
        <source>password cache</source>
        <translation>кэш паролей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="142"/>
        <source>reload</source>
        <translation>перезагрузить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="142"/>
        <source>restart</source>
        <translation>перезапустить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="142"/>
        <source>gpg-agent</source>
        <translation>gpg-agent</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="143"/>
        <source>maintenance</source>
        <translation>обслуживание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="148"/>
        <source>rPGP</source>
        <translation>rPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="148"/>
        <source>rust</source>
        <translation>rust</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="134"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="148"/>
        <source>engine</source>
        <translation>движок</translation>
    </message>
    <message>
        <source>Instant Messaging</source>
        <translation type="vanished">Мгновенные сообщения</translation>
    </message>
    <message>
        <source>message book</source>
        <translation type="vanished">Книга сообщений</translation>
    </message>
    <message>
        <source>phrase</source>
        <translation type="vanished">Фраза</translation>
    </message>
    <message>
        <source>fingerprint</source>
        <translation type="vanished">Отпечаток</translation>
    </message>
    <message>
        <source>token</source>
        <translation type="vanished">Токен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="154"/>
        <source>security level</source>
        <translation>Уровень безопасности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="154"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="154"/>
        <source>keychain</source>
        <translation>Связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="155"/>
        <source>log level</source>
        <translation>Уровень логирования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="155"/>
        <source>ring buffer</source>
        <translation>Кольцевой буфер</translation>
    </message>
    <message>
        <source>integrity check</source>
        <translation type="vanished">Проверка целостности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="155"/>
        <source>ENV.ini</source>
        <translation>ENV.ini</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="178"/>
        <source>Search settings…</source>
        <translation>Поиск настроек…</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="231"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="437"/>
        <source>Restart Required</source>
        <translation>Требуется перезапуск</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="438"/>
        <source>Some of your changes only take effect after GpgFrontend restarts.</source>
        <translation>Некоторые изменения вступают в силу только после перезапуска GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="440"/>
        <source>Some of your changes only take effect after the interface reloads.</source>
        <translation>Некоторые изменения вступают в силу только после перезагрузки интерфейса.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="443"/>
        <source>Pages with changes that need this: %1.

Choose Cancel to discard everything you changed in this dialog and keep the current settings.</source>
        <translation>Изменения, требующие этого, были сделаны: %1.

Нажмите «Отмена», чтобы отменить все изменения в этом диалоге и сохранить текущие настройки.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="448"/>
        <source>Save and Restart</source>
        <translation>Сохранить и перезапустить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="448"/>
        <source>Save and Reload</source>
        <translation>Сохранить и перезагрузить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="484"/>
        <source>Operations Still Running</source>
        <translation>Операции всё ещё выполняются</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="485"/>
        <source>Some key databases you changed or removed still have operations running:</source>
        <translation>В некоторых базах данных ключей, которые вы изменили или удалили, всё ещё выполняются операции:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="488"/>
        <source>They will finish against the key database as it was before. Apply the changes now?</source>
        <translation>Они завершатся с базой данных ключей в её прежнем состоянии. Применить изменения сейчас?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="603"/>
        <source>System Default</source>
        <translation>По умолчанию в системе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsDialog.cpp" line="221"/>
        <source>Preferences</source>
        <translation>Настройки</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SigningKeysPicker</name>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="191"/>
        <source>Select Signing Key(s)</source>
        <translation>Выберите ключ(и) подписи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="192"/>
        <source>Choose Signing Key(s) or Subkey(s)</source>
        <translation>Выберите ключ(и) подписи или подключ(и)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="193"/>
        <source>Select the private key(s) or specific signing subkey(s) to use for this operation. Expand a key to choose a particular subkey.</source>
        <translation>Выберите закрытый ключ(и) или конкретный подключ(и) подписи для этой операции. Разверните ключ, чтобы выбрать нужный подключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="196"/>
        <source>Confirm</source>
        <translation>Подтвердить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="196"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="196"/>
        <source>No Key Selected</source>
        <translation>Ключ не выбран</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/KeysPicker.cpp" line="197"/>
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
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="166"/>
        <source>Change Name</source>
        <translation>Изменить имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="168"/>
        <source>Change Language</source>
        <translation>Изменить язык</translation>
    </message>
    <message>
        <source>Change Gender</source>
        <translation type="vanished">Изменить пол</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="172"/>
        <source>Change Login Data</source>
        <translation>Изменить данные для входа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="174"/>
        <source>Change Public Key URL</source>
        <translation>Изменить URL открытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="179"/>
        <source>Change PIN</source>
        <translation>Изменить PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="181"/>
        <source>Change Admin PIN</source>
        <translation>Изменить PIN администратора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="183"/>
        <source>Change Reset Code</source>
        <translation>Изменить код сброса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="149"/>
        <source>Fetch</source>
        <translation>Получить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="126"/>
        <source>Slot</source>
        <translation>Слот</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="135"/>
        <source>Key Info (ki)</source>
        <translation>Информация о ключе (ki)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="136"/>
        <source>Additional Auth (aac)</source>
        <translation>Дополнительная аутентификация (aac)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="137"/>
        <source>Biometric Terminal (bt)</source>
        <translation>Биометрический терминал (bt)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="138"/>
        <source>KDF Supported</source>
        <translation>Поддержка KDF</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="188"/>
        <source>Restart All Gpg-Agents</source>
        <translation>Перезапустить все агенты GPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="147"/>
        <source>Generate Card Keys</source>
        <translation>Сгенерировать ключи карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="150"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <source>Operations</source>
        <translation type="vanished">Операции</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="206"/>
        <source>Successful Operation</source>
        <translation>Успешная операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="207"/>
        <source>Restart all the GnuPG&apos;s components successfully</source>
        <translation>Все компоненты GnuPG успешно перезапущены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="210"/>
        <source>Failed Operation</source>
        <translation>Неудавшаяся операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="211"/>
        <source>Failed to restart all or one of the GnuPG&apos;s component(s)</source>
        <translation>Не удалось перезапустить один или несколько компонентов GnuPG</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="241"/>
        <source>Generate card key failed.</source>
        <translation>Не удалось сгенерировать ключ карты.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="105"/>
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
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="110"/>
        <source>Reader</source>
        <translation>Считыватель</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="111"/>
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
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="112"/>
        <source>Manufacturer</source>
        <translation>Производитель</translation>
    </message>
    <message>
        <source>Card Holder</source>
        <translation type="vanished">Держатель карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="115"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="116"/>
        <source>Sex</source>
        <translation>Пол</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="119"/>
        <source>Signature Counter</source>
        <translation>Счётчик подписей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="120"/>
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
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="121"/>
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
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="362"/>
        <source>Enabled</source>
        <translation>Включено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="363"/>
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
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="139"/>
        <source>Status Indicator</source>
        <translation>Индикатор состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="143"/>
        <source>No card detected</source>
        <translation>Карта не обнаружена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="145"/>
        <source>Cardholder</source>
        <translation>Держатель карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="146"/>
        <source>Access Codes</source>
        <translation>Коды доступа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="148"/>
        <source>Move Key to Card</source>
        <translation>Переместить ключ на карту</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="151"/>
        <source>More Actions</source>
        <translation>Ещё действия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="170"/>
        <source>Change Sex</source>
        <translation>Изменить пол</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="215"/>
        <source>Open GnuPG Smart Card HOWTO</source>
        <translation>Открыть руководство GnuPG по смарт-картам</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="310"/>
        <source>Unnamed Card</source>
        <translation>Безымянная карта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="324"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="326"/>
        <source>%1, version %2</source>
        <translation>%1, версия %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="329"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="330"/>
        <source>Not set</source>
        <translation>Не задано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="333"/>
        <source>● Ready</source>
        <translation>● Готов</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="345"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="345"/>
        <source>Reset Code</source>
        <translation>Код сброса</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="346"/>
        <source>Admin PIN</source>
        <translation>PIN администратора</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="352"/>
        <source>%1 n/a</source>
        <translation>%1 н/д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="356"/>
        <source>%1 %2 left</source>
        <translation>Осталось %1 %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="359"/>
        <source>%1 retries left, maximum length %2</source>
        <translation>Осталось %1 попыток, макс. длина %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="365"/>
        <source>User Interaction Flag — Sign: %1 · Encrypt: %2 · Authenticate: %3</source>
        <translation>Флаг взаимодействия с пользователем — Подпись: %1 · Шифрование: %2 · Аутентификация: %3</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="410"/>
        <source>N/A</source>
        <translation>Н/Д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="486"/>
        <source>Read the GnuPG Smart Card HOWTO</source>
        <translation>Прочитать руководство GnuPG по смарт-картам</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="576"/>
        <source>No Card</source>
        <translation>Нет карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="577"/>
        <source>No smart card is currently selected.</source>
        <translation>В настоящее время смарт-карта не выбрана.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="584"/>
        <source>Not Supported</source>
        <translation>Не поддерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="585"/>
        <source>Moving a key to a smart card is only supported for GnuPG key databases. The rpgp engine is not supported for this feature.</source>
        <translation>Перемещение ключа на смарт-карту поддерживается только для баз данных ключей GnuPG. Движок rpgp не поддерживается для этой функции.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="607"/>
        <source>Cardholder&apos;s Surname</source>
        <translation>Фамилия держателя карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="608"/>
        <source>Please enter your surname (e.g., Lee):</source>
        <translation>Введите вашу фамилию (например, Иванов):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="613"/>
        <source>Cardholder&apos;s Given Name</source>
        <translation>Имя держателя карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="614"/>
        <source>Please enter your given name (e.g., Chris):</source>
        <translation>Введите ваше имя (например, Иван):</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="623"/>
        <source>Too Long</source>
        <translation>Слишком длинное</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="624"/>
        <source>Combined name too long (max 39 characters).</source>
        <translation>Комбинированное имя слишком длинное (максимум 39 символов).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="639"/>
        <source>Male</source>
        <translation>Мужской</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="639"/>
        <source>Female</source>
        <translation>Женский</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="643"/>
        <source>Select sex to store in &apos;%1&apos;</source>
        <translation>Выберите пол для сохранения в «%1»</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="658"/>
        <source>Enter new value for attribute &apos;%1&apos;</source>
        <translation>Введите новое значение для атрибута «%1»</translation>
    </message>
    <message>
        <source>Status</source>
        <translation type="vanished">Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="376"/>
        <source>Not enabled</source>
        <translation>Не включено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="379"/>
        <source>Enabled (no protection)</source>
        <translation>Включено (без защиты)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="382"/>
        <source>Enabled with salt protection</source>
        <translation>Включено с защитой солью</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="317"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="385"/>
        <source>Unknown</source>
        <translation>Неизвестно</translation>
    </message>
    <message>
        <source>Key Information</source>
        <translation type="vanished">Информация о ключе</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="396"/>
        <source>No key information available.</source>
        <translation>Информация о ключе недоступна.</translation>
    </message>
    <message>
        <source>No.</source>
        <translation type="vanished">№</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="127"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="126"/>
        <source>Created</source>
        <translation>Создано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="127"/>
        <source>Grip</source>
        <translation>Дескриптор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="126"/>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="107"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="113"/>
        <source>Card</source>
        <translation>Карта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="109"/>
        <source>Identity</source>
        <translation>Личность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="114"/>
        <source>Application</source>
        <translation>Приложение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="118"/>
        <source>Access &amp; Status</source>
        <translation>Доступ и статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="123"/>
        <source>Keys on Card</source>
        <translation>Ключи на карте</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="126"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="126"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <source>Curve</source>
        <translation type="vanished">Кривая</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="134"/>
        <source>Extended Capabilities</source>
        <translation>Расширенные возможности</translation>
    </message>
    <message>
        <source>Key Info (ki): %1</source>
        <translation type="vanished">Информация о ключе (ki): %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="371"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="425"/>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="371"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="426"/>
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
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="141"/>
        <source>Additional Info</source>
        <translation>Дополнительная информация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="468"/>
        <source>No OpenPGP Smart Card Found</source>
        <translation>Смарт-карта OpenPGP не найдена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="473"/>
        <source>No OpenPGP-compatible smart card has been detected.</source>
        <translation>Совместимая с OpenPGP смарт-карта не обнаружена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="474"/>
        <source>An OpenPGP Smart Card is a physical device that securely stores your private cryptographic keys and can be used for digital signing, encryption, and authentication. Popular examples include YubiKey, Nitrokey, and other GnuPG-compatible tokens.</source>
        <translation>Смарт-карта OpenPGP — это физическое устройство для безопасного хранения закрытых криптографических ключей, которое может использоваться для цифровой подписи, шифрования и аутентификации. Примеры: YubiKey, Nitrokey и другие совместимые с GnuPG токены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="480"/>
        <source>Make sure your card is inserted and properly recognized by the system. You can also try reconnecting the card or restarting the application.</source>
        <translation>Убедитесь, что карта вставлена и правильно распознана системой. Также попробуйте переподключить карту или перезапустить приложение.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="495"/>
        <source>Note: Smart card support of GpgFrontend requires GnuPG version 2.3.0 or later.</source>
        <translation>Примечание: поддержка смарт-карт в GpgFrontend требует GnuPG версии 2.3.0 или выше.</translation>
    </message>
    <message>
        <source>Read the GnuPG Smart Card HOWTO: </source>
        <translation type="vanished">Читайте руководство по смарт-картам GnuPG: </translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="642"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="657"/>
        <source>Modify Card Attribute</source>
        <translation>Изменить атрибут карты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="676"/>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="690"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="677"/>
        <source>Attribute operation completed successfully.</source>
        <translation>Операция с атрибутом успешно выполнена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="240"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/SmartCardControllerDialog.cpp" line="691"/>
        <source>PIN operation completed successfully.</source>
        <translation>Операция с PIN успешно выполнена.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::StatusDockPresenter</name>
    <message>
        <location filename="../../../../src/ui/widgets/StatusDockPresenter.cpp" line="522"/>
        <source>Expand</source>
        <translation>Развернуть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusDockPresenter.cpp" line="522"/>
        <source>Collapse</source>
        <translation>Свернуть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusDockPresenter.cpp" line="523"/>
        <location filename="../../../../src/ui/widgets/StatusDockPresenter.cpp" line="539"/>
        <source>Expand Status Panel</source>
        <translation>Развернуть панель состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusDockPresenter.cpp" line="524"/>
        <source>Collapse Status Panel</source>
        <translation>Свернуть панель состояния</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusDockPresenter.cpp" line="574"/>
        <source>Close Status Panel</source>
        <translation>Закрыть панель состояния</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::StatusIndicatorInfo</name>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="57"/>
        <source>Click to manage profiles.</source>
        <translation>Нажмите, чтобы управлять профилями.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="62"/>
        <source>Click to see the full status.</source>
        <translation>Нажмите, чтобы увидеть полный статус.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="74"/>
        <source>Profile</source>
        <translation>Профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="82"/>
        <source>%1 (temporary)</source>
        <translation>%1 (временный)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="89"/>
        <source>Opened from a file, and not kept on this computer. Closing asks whether to save the changes back into it.</source>
        <translation>Открыт из файла и не хранится на этом компьютере. При закрытии будет предложено сохранить изменения обратно в него.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="94"/>
        <source>Not kept on this computer. Everything in it is destroyed when this window closes.</source>
        <translation>Не хранится на этом компьютере. Всё содержимое уничтожается при закрытии этого окна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="99"/>
        <source>This window&apos;s profile — its own settings, keys and saved state</source>
        <translation>Профиль этого окна — его собственные настройки, ключи и сохранённое состояние</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="110"/>
        <source>Kept in %1.</source>
        <translation>Хранится в %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="118"/>
        <source>This profile&apos;s own key is held in memory only.</source>
        <translation>Собственный ключ этого профиля хранится только в памяти.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="142"/>
        <source>Current OpenPGP backend and version</source>
        <translation>Текущий бэкенд OpenPGP и версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="146"/>
        <source>Key database: %1</source>
        <translation>База данных ключей: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="163"/>
        <source>Portable Mode</source>
        <translation>Портативный режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="165"/>
        <source>Installed Mode</source>
        <translation>Установленный режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="170"/>
        <source>Running from the folder it was unpacked into, taking its settings and keys along with it.</source>
        <translation>Запуск из папки, в которую он был распакован, с переносом своих настроек и ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="174"/>
        <source>Installed on this computer, with its settings and keys kept in this user&apos;s data folder.</source>
        <translation>Установлен на этом компьютере, с сохранением настроек и ключей в папке данных этого пользователя.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/StatusIndicatorInfo.cpp" line="178"/>
        <source>This profile keeps its own keys, separate from the rest of the computer.</source>
        <translation>Этот профиль хранит свои собственные ключи отдельно от остальной части компьютера.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::StatusTab</name>
    <message>
        <source>Application Status</source>
        <translation type="vanished">Статус приложения</translation>
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
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="488"/>
        <source>Portable Mode</source>
        <translation>Портативный режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="488"/>
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
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="614"/>
        <source>Active</source>
        <translation>Активен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="614"/>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="623"/>
        <source>Default Pinentry Program</source>
        <translation>Программа ввода PIN по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="464"/>
        <source>Secure Level:</source>
        <translation>Уровень защиты:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="461"/>
        <source>Application Key Protection:</source>
        <translation>Защита ключей приложения:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="468"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="490"/>
        <source>System Credential Store:</source>
        <translation>Системное хранилище учетных данных:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="469"/>
        <source>Unavailable</source>
        <translation>Недоступно</translation>
    </message>
    <message>
        <source>Credential Store Detail:</source>
        <translation type="vanished">Детали хранилища учетных данных:</translation>
    </message>
    <message>
        <source>Security Level:</source>
        <translation type="vanished">Уровень безопасности:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="487"/>
        <source>Running Mode:</source>
        <translation>Режим запуска:</translation>
    </message>
    <message>
        <source>Self-Check Status:</source>
        <translation type="vanished">Статус самопроверки:</translation>
    </message>
    <message>
        <source>GnuPG Home Path Detail:</source>
        <translation type="vanished">Детали домашнего пути GnuPG:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="613"/>
        <source>GnuPG Offline Mode:</source>
        <translation>Автономный режим GnuPG:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="621"/>
        <source>Pinentry Program Path:</source>
        <translation>Путь к программе ввода PIN:</translation>
    </message>
    <message>
        <source>Profile:</source>
        <translation type="vanished">Профиль:</translation>
    </message>
    <message>
        <source>Profile Type:</source>
        <translation type="vanished">Тип профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="580"/>
        <source>Profile Folder:</source>
        <translation>Папка профиля:</translation>
    </message>
    <message>
        <source>Keys:</source>
        <translation type="vanished">Ключи:</translation>
    </message>
    <message>
        <source>Inside this profile</source>
        <translation type="vanished">Внутри этого профиля</translation>
    </message>
    <message>
        <source>System keyring</source>
        <translation type="vanished">Системная связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="587"/>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="589"/>
        <source>Workspace:</source>
        <translation>Рабочая область:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="587"/>
        <source>None</source>
        <translation>Нет</translation>
    </message>
    <message>
        <source>Profile Layout Version:</source>
        <translation type="vanished">Версия макета профиля:</translation>
    </message>
    <message>
        <source>Imported From Package:</source>
        <translation type="vanished">Импортировано из пакета:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="599"/>
        <source>Profiles Folder:</source>
        <translation>Папка профилей:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="574"/>
        <source>Profile</source>
        <translation>Профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="479"/>
        <source>GnuPG Home:</source>
        <translation>Домашний каталог GnuPG:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="480"/>
        <source>Unusable</source>
        <translation>Непригоден</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="494"/>
        <source>Application</source>
        <translation>Приложение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="502"/>
        <source>Profile ID:</source>
        <translation>Идентификатор профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="513"/>
        <source>Session Storage:</source>
        <translation>Хранилище сеанса:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="524"/>
        <source>Profile Key:</source>
        <translation>Ключ профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="525"/>
        <source>Held in memory only</source>
        <translation>Хранится только в памяти</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="526"/>
        <source>The key that protects this profile&apos;s own saved data is never written here. Your OpenPGP keys are a separate thing and do live in the session storage above, because GnuPG needs real files for them.</source>
        <translation>Ключ, защищающий собственные сохранённые данные этого профиля, никогда не записывается сюда. Ваши ключи OpenPGP — это отдельная вещь, они действительно находятся в указанном выше хранилище сеанса, потому что GnuPG требует для них реальных файлов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="539"/>
        <source>Profile Key Rotation:</source>
        <translation>Ротация ключа профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="540"/>
        <source>On a schedule</source>
        <translation>По расписанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="541"/>
        <source>New saved data uses the current period&apos;s key, and the keys that open what earlier periods wrote are kept alongside it.</source>
        <translation>Новые сохраняемые данные используют ключ текущего периода, а ключи, открывающие данные, записанные в более ранние периоды, хравтся рядом с ним.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="551"/>
        <source>Profile Format Version:</source>
        <translation>Версия формата профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="561"/>
        <source>Profile File:</source>
        <translation>Файл профиля:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="570"/>
        <source>Imported From:</source>
        <translation>Импортивровано из:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="603"/>
        <source>Folders</source>
        <translation>Папки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="609"/>
        <source>Available</source>
        <translation>Достуен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="628"/>
        <source>OpenPGP Engines</source>
        <translation>Движки OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="649"/>
        <source>Needs Attention</source>
        <translation>Требует внимания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="650"/>
        <source>At a Glance</source>
        <translation>Краткий обзор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="667"/>
        <source>More details</source>
        <translation>Подробнее</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="667"/>
        <source>Details</source>
        <translation>Сведения</translation>
    </message>
    <message>
        <source>Supported OpenPGP Engines</source>
        <translation type="vanished">Поддерживаемые движки OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="672"/>
        <source>Copy Status Information</source>
        <translation>Копировать сведения о состоянии</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="680"/>
        <source>These values reflect the current startup environment and may help when reporting issues.</source>
        <translation>Эти значения отражают текущее окружение запуска и могут помочь при сообщении о проблемах.</translation>
    </message>
    <message>
        <source>Tip: These values reflect the current startup environment and may help when reporting issues.</source>
        <translation type="vanished">Подсказка: Эти значения отражают текущую среду запуска и могут помочь при сообщении о проблемах.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::SubkeyGenerateDialog</name>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="157"/>
        <source>Tipps: if the key pair has a passphrase, the subkey&apos;s passphrase must be equal to it.</source>
        <translation>Подсказка: если у пары ключей есть парольная фраза, парольная фраза подключа должна совпадать с ней.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="173"/>
        <source>Generate New Subkey</source>
        <translation>Сгенерировать новый подключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="133"/>
        <source>Authentication</source>
        <translation>Аутентификация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="127"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="128"/>
        <source>Key Length</source>
        <translation>Длина ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="129"/>
        <source>Expire Date</source>
        <translation>Дата окончания</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="130"/>
        <source>Usage</source>
        <translation>Использование</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="131"/>
        <source>Encrypt</source>
        <translation>Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="132"/>
        <source>Sign</source>
        <translation>Подписать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="134"/>
        <source>Non Expired</source>
        <translation>Действующий</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="135"/>
        <source>No Passphrase</source>
        <translation>Без парольной фразы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="136"/>
        <source>Second Algorithm</source>
        <translation>Алгоритм подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="137"/>
        <source>Second Key Length</source>
        <translation>Длина подключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="168"/>
        <source>Note: post-quantum (PQC) subkey algorithms are unavailable here because the primary key uses the v4 key format. Generate a v6 key to use PQC algorithms.</source>
        <translation>Примечание: постквантовые (PQC) алгоритмы подключа недоступны, так как первичный ключ использует формат ключа v4. Сгенерируйте ключ v6 для использования алгоритмов PQC.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="479"/>
        <source>Please give a valid subkey algorithm.</source>
        <translation>Укажите допустимый алгоритм подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="489"/>
        <source>Please give a valid second algorithm.</source>
        <translation>Укажите допустимый алгоритм подключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="499"/>
        <source>Time to subkey expiration must not be less than 120 seconds.</source>
        <translation>Время до истечения подключа должно быть не менее 120 секунд.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="516"/>
        <source>Generating</source>
        <translation>Генерация</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="530"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="531"/>
        <source>Unknown error occurred</source>
        <translation>Произошла неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="561"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/key_generate/SubkeyGenerateDialog.cpp" line="562"/>
        <source>Subkey generation completed successfully.</source>
        <translation>Генерация подключа выполнена успешно.</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TextEdit</name>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="185"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="214"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="660"/>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="186"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="215"/>
        <source>Cannot read file %1:
%2.</source>
        <translation>Невозможно прочитать файл %1:
%2.</translation>
    </message>
    <message>
        <source>Open file</source>
        <translation type="vanished">Открыть файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="96"/>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="104"/>
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
        <source>File Open Error</source>
        <translation type="vanished">Ошибка открытия файла</translation>
    </message>
    <message>
        <source>The file &quot;%1&quot; could not be opened.</source>
        <translation type="vanished">Не удалось открыть файл «%1».</translation>
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
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="128"/>
        <source>Unknown Tab Type</source>
        <translation>Неизвестный тип вкладки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="129"/>
        <source>The current tab has an unknown type. Cannot save.</source>
        <translation>Текущая вкладка имеет неизвестный тип. Сохранение невозможно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="148"/>
        <source>Unsupported Operation</source>
        <translation>Неподдерживаемая операция</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="149"/>
        <source>Saving is not supported for tabs of type &apos;%1&apos;.</source>
        <translation>Операция сохранения файла для типа вкладки «%1» не поддерживается.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="254"/>
        <source>Save File</source>
        <translation>Сохранить файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="339"/>
        <source>Unsaved Document</source>
        <translation>Несохранённый документ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="340"/>
        <source>The document &quot;%1&quot; has been modified. Do you want to save your changes?</source>
        <translation>Документ «%1» был изменён. Сохранить изменения?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="343"/>
        <source>Note:</source>
        <translation>Примечание:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="344"/>
        <source>If you don&apos;t save, your changes will be lost.</source>
        <translation>Если вы не сохраните эти файлы, все изменения будут потеряны.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEdit.cpp" line="660"/>
        <source>No document to print.</source>
        <translation>Нет документа для печати</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TextEditTabWidget</name>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="352"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="370"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="375"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="402"/>
        <source>File Open Error</source>
        <translation>Ошибка открытия файла</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="298"/>
        <source>The file &quot;%1&quot; could not be opened.</source>
        <translation>Не удалось открыть файл «%1».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="171"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="172"/>
        <source>Copy Path</source>
        <translation>Копировать путь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="173"/>
        <source>Reveal in File Browser</source>
        <translation>Показать в файловом менеджере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="278"/>
        <source>The file does not exist.</source>
        <translation>Файл не существует.</translation>
    </message>
    <message>
        <source>The file &quot;%1&quot; is larger than 1 MB and will not be opened in the text editor.</source>
        <translation type="vanished">Файл «%1» превышает 1 МБ и не будет открыт в текстовом редакторе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="287"/>
        <source>The file &quot;%1&quot; is larger than 4 MB and will not be opened in the text editor.</source>
        <translation>Файл «%1» превышает 4 МБ и не будет открыт в текстовом редакторе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="306"/>
        <source>The file &quot;%1&quot; appears to be a binary file and will not be opened.</source>
        <translation>Файл «%1» является двоичным файлом и не будет открыт.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="353"/>
        <source>The file &quot;%1&quot; does not exist.</source>
        <translation>Файл «%1» не существует.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="575"/>
        <source>Select Default Path</source>
        <translation>Выбрать путь по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="645"/>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1201"/>
        <source>Workspace</source>
        <translation>Рабочая область</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1173"/>
        <source>Directory Permission Denied</source>
        <translation>Доступ к каталогу запрещён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1174"/>
        <source>You do not have permission to access the directory &quot;%1&quot;.</source>
        <translation>У вас нет прав доступа к каталогу «%1».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/TextEditTabWidget.cpp" line="1196"/>
        <source>Root</source>
        <translation>Корень</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::TranslatorsTab</name>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="387"/>
        <source>Thanks to all translators</source>
        <translation>Спасибо всем переводчикам</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="397"/>
        <source>Translator information is not available.</source>
        <translation>Информация о переводчике недоступна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="401"/>
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
        <location filename="../../../../src/ui/dialog/WaitingDialog.cpp" line="82"/>
        <source>Cancelling...</source>
        <translation>Отменяется...</translation>
    </message>
</context>
<context>
    <name>GpgFrontend::UI::Wizard</name>
    <message>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="222"/>
        <location filename="../../../../src/ui/dialog/Wizard.cpp" line="229"/>
        <source>Welcome to GpgFrontend</source>
        <translation>Добро пожаловать в GpgFrontend</translation>
    </message>
</context>
<context>
    <name>GpgKey</name>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="108"/>
        <source>Unknown</source>
        <translation>Неизвестно</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="110"/>
        <source>Undefined</source>
        <translation>Не определено</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="112"/>
        <source>Never</source>
        <translation>Никогда</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="114"/>
        <source>Marginal</source>
        <translation>Частичное</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="116"/>
        <source>Full</source>
        <translation>Полное</translation>
    </message>
    <message>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="104"/>
        <location filename="../../../../src/core/model/GpgKey.cpp" line="118"/>
        <source>Ultimate</source>
        <translation>Абсолютное</translation>
    </message>
</context>
<context>
    <name>ProfileAccessor</name>
    <message>
        <location filename="../../../../src/core/profile/ProfileAccessor.cpp" line="135"/>
        <source>an ordinary folder on this disk</source>
        <translation>обычная папка на этом диске</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProtectedFsProfileAccessor.cpp" line="236"/>
        <source>memory only; not written to your disk in the normal course of things</source>
        <translation>только в памяти; не записывается на диск в обычном режиме</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProtectedFsProfileAccessor.cpp" line="242"/>
        <source>an encrypted folder this session alone can read</source>
        <translation>зашифрованная папка, которую может прочитать только этот сеанс</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProtectedFsProfileAccessor.cpp" line="246"/>
        <source>an ordinary temporary folder on this disk</source>
        <translation>обычная временная папка на этом диске</translation>
    </message>
</context>
<context>
    <name>ProfileSecureKeyManager</name>
    <message>
        <location filename="../../../../src/core/profile/ProfileSecureKeyManager.cpp" line="439"/>
        <source>this session&apos;s memory, and nowhere on this machine</source>
        <translation>в памяти этого сеанса и нигде на этом компьютере</translation>
    </message>
</context>
<context>
    <name>QCoreApplication</name>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="208"/>
        <source>File Hash Information</source>
        <translation>Хеш-информация о файле</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="182"/>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="225"/>
        <source>Filename</source>
        <translation>Имя файла</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="210"/>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="225"/>
        <source>: </source>
        <translation>: </translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="183"/>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="185"/>
        <source>File Size</source>
        <translation>Размер файла</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="223"/>
        <source>Error: cannot read target file</source>
        <translation>Ошибка: не удалось прочитать целевой файл</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="257"/>
        <source>target path doesn&apos;t exists</source>
        <translation>целевой путь не существует</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="263"/>
        <source>do NOT have permission to write path</source>
        <translation>нет прав на запись по указанному пути</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="269"/>
        <source>do NOT have permission to read/write file</source>
        <translation>нет прав на чтение/запись файла</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/IOUtils.cpp" line="272"/>
        <source>Success</source>
        <translation>Успешно</translation>
    </message>
    <message>
        <source>Sync All Public Key</source>
        <translation type="vanished">Синхронизировать все открытые ключи</translation>
    </message>
    <message>
        <source>You have not checked any public keys that you want to synchronize, do you want to synchronize all local public keys from the key server?</source>
        <translation type="vanished">Вы не отметили открытые ключи для синхронизации. Хотите синхронизировать все локальные открытые ключи с сервера ключей?</translation>
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
        <location filename="../../../../src/core/GFCoreInit.cpp" line="558"/>
        <source>Basic Path Initiation Failed</source>
        <translation>Не удалось инициализировать базовые пути</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="590"/>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="679"/>
        <source>No Supported OpenPGP Engine Detected</source>
        <translation>Не обнаружено поддерживаемого движка OpenPGP</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="635"/>
        <source>No valid Key Database</source>
        <translation>Нет допустимой базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="716"/>
        <source>GpgME Default Context Initiation Failed</source>
        <translation>Не удалось инициализировать контекст GpgME по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="738"/>
        <source>Gpg Default Key Database Initiation Failed</source>
        <translation>Не удалось инициализировать базу данных ключей GPG по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="1060"/>
        <source>The key database &quot;%1&quot; was still in use, so its folder was not renamed. It keeps its previous name.</source>
        <translation>База данных ключей &quot;%1&quot; всё ещё использовалась, поэтому её папка не была переименована. Она сохраняет прежнее имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="1078"/>
        <source>The folder of the key database &quot;%1&quot; could not be renamed. It keeps its previous name.</source>
        <translation>Не удалось переименовать папку базы данных ключей &quot;%1&quot;. Она сохраняет прежнее имя.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="1234"/>
        <source>The default key database &quot;%1&quot; could not be opened.</source>
        <translation>Не удалось открыть базу данных ключей по умолчанию &quot;%1&quot;.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/GFCoreInit.cpp" line="1244"/>
        <source>No key database could be opened.</source>
        <translation>Не удалось открыть ни одну базу данных ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="75"/>
        <source>Starting up...</source>
        <translation>Запуск...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="77"/>
        <source>Checking the GnuPG environment...</source>
        <translation>Проверка среды GnuPG...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="79"/>
        <source>Checking the rPGP engine...</source>
        <translation>Проверка движка rPGP...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="81"/>
        <source>Resolving GnuPG paths...</source>
        <translation>Определение путей GnuPG...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="83"/>
        <source>Preparing the OpenPGP backend engine...</source>
        <translation>Подготовка внутреннего движка OpenPGP...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="85"/>
        <source>Building the default engine context...</source>
        <translation>Создание контекста движка по умолчанию...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="88"/>
        <source>Loading key databases...</source>
        <translation>Загрузка баз данных ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="92"/>
        <source>Loading key database &quot;%1&quot;...</source>
        <translation>Загрузка базы данных ключей «%1»...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="95"/>
        <source>Scanning modules...</source>
        <translation>Сканирование модулей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="97"/>
        <source>Verifying modules...</source>
        <translation>Проверка модулей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="100"/>
        <source>Loading modules...</source>
        <translation>Загрузка модулей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="101"/>
        <source>Loading module &quot;%1&quot;...</source>
        <translation>Загрузка модуля «%1»...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="104"/>
        <source>Ready.</source>
        <translation>Готово.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="178"/>
        <source>Starting GpgFrontend</source>
        <translation>Запуск GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="139"/>
        <source>Preparing OpenPGP Environment</source>
        <translation>Подготовка среды OpenPGP</translation>
    </message>
    <message>
        <source>Loading...</source>
        <translation type="vanished">Загрузка...</translation>
    </message>
    <message>
        <source>Loading essential information</source>
        <translation type="vanished">Загрузка основной информации</translation>
    </message>
    <message>
        <source>GpgFrontend is checking your OpenPGP environment and preparing the default engine. This may take a few seconds.</source>
        <translation type="vanished">GpgFrontend проверяет вашу среду OpenPGP и подготавливает движок по умолчанию. Это может занять несколько секунд.</translation>
    </message>
    <message>
        <source>Please keep this window open while the initialization is running.</source>
        <translation type="vanished">Пожалуйста, не закрывайте это окно во время инициализации.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/GpgFrontendUIInit.cpp" line="236"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/core/utils/GpgUtils.cpp" line="1300"/>
        <source>Built-in</source>
        <translation>Встроенный</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../../../../src/GpgFrontend.cpp" line="148"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="279"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="303"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="232"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="643"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="795"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="862"/>
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
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="396"/>
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
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="431"/>
        <source>Secure Key Generation Failed</source>
        <translation>Не удалось сгенерировать безопасный ключ</translation>
    </message>
    <message>
        <source>Failed to compute key ID.</source>
        <translation type="vanished">Не удалось вычислить идентификатор ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="397"/>
        <source>The application secure key could not be saved: %1</source>
        <translation>Не удалось сохранить безопасный ключ приложения: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="400"/>
        <source>Anything saved now would be unreadable after a restart, so the application will not continue. Please check your storage and permissions.</source>
        <translation>Сохранённое сейчас будет нечитаемо после перезапуска, поэтому приложение не будет продолжено. Проверьте хранилище и разрешения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="410"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="419"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="439"/>
        <source>App Secure Key Error</source>
        <translation>Ошибка безопасного ключа приложения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="411"/>
        <source>Failed to decrypt the application secure key. The key file may be corrupted.</source>
        <translation>Не удалось расшифровать безопасный ключ приложения. Возможно, файл ключа повреждён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="413"/>
        <source>Please clear the secure key and try again.</source>
        <translation>Пожалуйста, очистите безопасный ключ и повторите попытку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="420"/>
        <source>Failed to read the application secure key from disk at: %1</source>
        <translation>Не удалось прочитать безопасный ключ приложения с диска: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="424"/>
        <source>Please ensure the key file exists and is accessible, or try re-initializing the secure key.</source>
        <translation>Убедитесь, что файл ключа существует и доступен, или попробуйте повторно инициализировать безопасный ключ.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="432"/>
        <source>Failed to generate an application secure key.</source>
        <translation>Не удалось сгенерировать безопасный ключ приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="433"/>
        <source>Please check your system&apos;s cryptography support.</source>
        <translation>Проверьте поддержку криптографии в вашей системе.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="232"/>
        <source>Reset Secure Key</source>
        <translation>Сбросить безопасный ключ</translation>
    </message>
    <message>
        <source>The key file at %1 could not be removed.</source>
        <translation type="vanished">Не удалось удалить файл ключа: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="459"/>
        <source>System Keychain Unavailable</source>
        <translation>Связка ключей системы недоступна</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="460"/>
        <source>The application key could not be protected using the system keychain, so it remains stored unprotected.</source>
        <translation>Не удалось защитить ключ приложения с помощью системной связки ключей, поэтому он остаётся незащищённым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="463"/>
        <source>This setting has been turned off. You can turn it on again once a keychain is available.</source>
        <translation>Этот параметр отключён. Вы можете включить его снова, когда связка ключей станет доступна.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="440"/>
        <source>The application secure key at %1 could not be read or rewritten.</source>
        <translation>Не удалось прочитать или перезаписать безопасный ключ приложения: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="244"/>
        <source>Secure Key Missing</source>
        <translation>Безопасный ключ отсутствует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="245"/>
        <source>The application key file is missing, but this profile still holds settings and data encrypted with it.</source>
        <translation>Файл ключа приложения отсутствует, но этот профиль всё ещё содержит настройки и данные, зашифрованные с его помощью.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="249"/>
        <source>This usually means the data folder was restored or copied without the key file.</source>
        <translation>Обычно это означает, что папка данных была восстановлена или скопирована без файла ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="252"/>
        <source>You can quit, put the key file back from your backup, and start the application again. Creating a new key instead lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Вы можете выйти, вернуть файл ключа из резервной копии и снова запустить приложение. Если вместо этого создать новый ключ, приложение запустится, но всё, что было зашифровано ранее, станет навсегда недоступным для чтения.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="256"/>
        <source>Expected key file: %1</source>
        <translation>Ожидаемый файл ключа: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="259"/>
        <source>Create New Key</source>
        <translation>Создать новый ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="296"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="443"/>
        <source>Please check your storage and permissions.</source>
        <translation>Проверьте хранилище и права доступа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="208"/>
        <source>Secure Key Unavailable</source>
        <translation>Безопасный ключ недоступен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="164"/>
        <source>Process %1 on %2</source>
        <translation>Процесс %1 на %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="168"/>
        <source>Another process</source>
        <translation>Другой процесс</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="175"/>
        <source>Held by</source>
        <translation>Удерживается</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="181"/>
        <source>Only do this if you are certain no other GpgFrontend window has this profile open. If one does, both copies will corrupt the profile&apos;s stored data.</source>
        <translation>Делайте это, только если вы уверены, что этот профиль не открыт ни в одном другом окне GpgFrontend. В противном случае обе копии повредят сохранённые данные профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="209"/>
        <source>The application key is protected by a secret kept in the system keychain, and that secret could not be read.</source>
        <translation>Ключ приложения защищён секретом, хранящимся в системной связке ключей, и этот секрет не удалось прочитать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="213"/>
        <source>This usually means the keychain is locked, was reset, or this profile was copied from another computer or user account.</source>
        <translation>Обычно это означает, что связка ключей заблокирована, была сброшена или этот профиль был скопирован с другого компьютера или учётной записи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="217"/>
        <source>You can unlock the keychain and start the application again. Resetting the key instead lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Вы можете разблокировать связку ключей и запустить приложение снова. Сброс ключа вместо этого позволит приложению запуститься, но всё, что оно ранее зашифровало, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <source>Keychain backend: %1</source>
        <translation type="vanished">Бэкенд связки ключей: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="196"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="231"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="258"/>
        <source>Quit</source>
        <translation>Выйти</translation>
    </message>
    <message>
        <source>Everything the application has encrypted with the old key will be permanently unreadable.</source>
        <translation type="vanished">Всё, что приложение зашифровало старым ключом, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="87"/>
        <source>Reset the secure key?</source>
        <translation>Сбросить безопасный ключ?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="60"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="76"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="84"/>
        <source>Reset to Default</source>
        <translation>Сбросить к значениям по умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="62"/>
        <source>Resetting removes the PIN and lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Сброс удаляет PIN и позволяет приложению запуститься, но всё, что оно ранее зашифровало, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="65"/>
        <source>Resetting lets the application start, but everything it previously encrypted becomes permanently unreadable.</source>
        <translation>Сброс позволяет запустить приложение, но всё, что было ранее зашифровано, станет навсегда нечитаемым.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="70"/>
        <source>Only do this if you cannot recall the PIN. There is no other way to recover the key.</source>
        <translation>Делайте это только в том случае, если вы не можете вспомнить PIN. Другого способа восстановить ключ нет.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="72"/>
        <source>You can unlock the keychain and start the application again instead. That costs nothing and leaves the key intact.</source>
        <translation>Вы можете разблокировать связку ключей и запустить приложение снова. Это ничего не стоит и сохраняет ключ в целости.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="75"/>
        <source>Go Back</source>
        <translation>Назад</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="85"/>
        <source>Everything the application has encrypted with the current key will be permanently unreadable.</source>
        <translation>Всё, что приложение зашифровало текущим ключом, станет навсегда недоступно для чтения.</translation>
    </message>
    <message>
        <source>Open Profile</source>
        <translation type="vanished">Открыть профиль</translation>
    </message>
    <message>
        <source>That passphrase did not open this file.</source>
        <translation type="vanished">Эта парольная фраза не открыла данный файл.</translation>
    </message>
    <message>
        <source>Enter the passphrase that protects this file:</source>
        <translation type="vanished">Введите парольную фразу, защищающую этот файл:</translation>
    </message>
    <message>
        <source>It is open in process %1 on %2.</source>
        <translation type="vanished">Он открыт в процессе %1 на %2.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="166"/>
        <source>this computer</source>
        <translation>этом компьютере</translation>
    </message>
    <message>
        <source>Another process has it open.</source>
        <translation type="vanished">Другой процесс уже открыл его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="171"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="285"/>
        <source>Profile Is Already Open</source>
        <translation>Профиль уже открыт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="286"/>
        <source>This profile is already open in another window.</source>
        <translation>Этот профиль уже открыт в другом окне.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="172"/>
        <source>Opening it twice would corrupt its stored data.</source>
        <translation>Открытие его дважды приведёт к повреждению сохранённых данных.</translation>
    </message>
    <message>
        <source>Profile: %1</source>
        <translation type="vanished">Профиль: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="197"/>
        <source>Force Unlock</source>
        <translation>Принудительная разблокировка</translation>
    </message>
    <message>
        <source>Only do this if you are certain no other GpgFrontend window has this profile open.</source>
        <translation type="vanished">Делайте это только в том случае, если вы уверены, что ни одно другое окно GpgFrontend не открыло этот профиль.</translation>
    </message>
    <message>
        <source>If one does, both copies will corrupt the profile&apos;s stored data.</source>
        <translation type="vanished">Если это произойдёт, обе копии повредят сохранённые данные профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="293"/>
        <source>Cannot Lock Profile</source>
        <translation>Не удаётся заблокировать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="294"/>
        <source>The lock file at %1 could not be created.</source>
        <translation>Не удалось создать файл блокировки %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="311"/>
        <source>This profile is set to open only in storage that is not left readable on this disk, and none is available here.</source>
        <translation>Этот профиль настроен на открытие только в хранилище, которое не оставляет данных читаемыми на этом диске, и здесь такое хранилище недоступно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="316"/>
        <source>Change &apos;Profile package storage&apos; in Settings to allow an ordinary temporary folder.</source>
        <translation>Измените «Хранилище пакетов профиля» в настройках, чтобы разрешить обычную временную папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="320"/>
        <source>Cannot Open Profile Safely</source>
        <translation>Не удаётся безопасно открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="327"/>
        <source>Not Enough Room</source>
        <translation>Недостаточно места</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="328"/>
        <source>There was not enough room to unpack this profile.</source>
        <translation>Не хватило места для распаковки этого профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="331"/>
        <source>Storage: %1</source>
        <translation>Хранилище: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="333"/>
        <source>The file itself is fine — free some space and try again.</source>
        <translation>Сам файл в порядке — освободите немного места и попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="340"/>
        <source>This Profile File Is Too Large</source>
        <translation>Этот файл профиля слишком большой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="341"/>
        <source>Opening a profile file needs to hold all of it in memory at once, and this one is bigger than this version can manage.</source>
        <translation>Для открытия файла профиля необходимо загрузить его в память целиком, а этот файл больше, чем может обработать данная версия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="347"/>
        <source>Size / limit: %1</source>
        <translation>Размер / лимит: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="349"/>
        <source>Nothing has been unpacked. This version cannot create a file this large either, so it was made somewhere else or has been damaged.</source>
        <translation>Ничего не распаковано. Эта версия также не может создать такой большой файл, поэтому он был создан в другом месте или повреждён.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="356"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="128"/>
        <source>This File Has Been Altered</source>
        <translation>Этот файл был изменён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="363"/>
        <source>a newer version</source>
        <translation>более новая версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="367"/>
        <source>Profile Is Too New</source>
        <translation>Профиль слишком новый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="368"/>
        <source>Profile Cannot Be Opened</source>
        <translation>Невозможно открыть профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="369"/>
        <source>This application data was last used by %1, which stores it in a format this version does not understand.</source>
        <translation>Эти данные приложения последний раз использовались %1, который сохраняет их в формате, который эта версия не понимает.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="373"/>
        <source>Continuing would damage it. Please use %1 or later, or start this version with a different profile.</source>
        <translation>Продолжение может повредить профиль. Используйте %1 или новее, или запустите эту версию с другим профилем.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="383"/>
        <source>Profile Upgrade Failed</source>
        <translation>Не удалось обновить профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="384"/>
        <source>Upgrading this profile stopped at step &apos;%1&apos;.</source>
        <translation>Обновление этого профиля остановилось на шаге &apos;%1&apos;.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="387"/>
        <source>The profile is intact. Please report this.</source>
        <translation>Профиль не повреждён. Пожалуйста, сообщите об этом.</translation>
    </message>
    <message>
        <source>Continuing would damage it. Please use %1 or later, or start this version with a different data folder.</source>
        <translation type="vanished">Продолжение может повредить данные. Используйте %1 или новее, или запустите эту версию с другой папкой данных.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="376"/>
        <source>Data folder: %1</source>
        <translation>Папка данных: %1</translation>
    </message>
    <message>
        <source>The application secure key at %1 could not be read.</source>
        <translation type="vanished">Не удалось прочитать ключ безопасности приложения по пути %1.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="480"/>
        <source>Application Key Not Protected</source>
        <translation>Ключ приложения не защищён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="481"/>
        <source>A PIN is configured, but the application key on disk is not encrypted.</source>
        <translation>Установлен PIN, но ключ приложения на диске не зашифрован.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="484"/>
        <source>This can happen if a previous change was interrupted. The PIN setting has been turned off and the key is left as it is.</source>
        <translation>Это может произойти, если предыдущее изменение было прервано. Настройка PIN отключена, и ключ оставлен как есть.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="130"/>
        <source>That PIN did not unlock the application key. Please try again.</source>
        <translation>Этот PIN-код не разблокировал ключ приложения. Пожалуйста, попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="133"/>
        <source>If you have forgotten your PIN, the application key and everything encrypted with it cannot be recovered.</source>
        <translation>Если вы забыли PIN-код, ключ приложения и всё, что с его помощью зашифровано, восстановить невозможно.</translation>
    </message>
    <message>
        <source>Failed to decrypt the application secure key. Your PIN may be incorrect, or the key file may be corrupted.Please clear the secure key and try again.</source>
        <translation type="vanished">Не удалось расшифровать ключ безопасности приложения. Возможно, вы ввели неверный PIN-код или файл ключа повреждён. Пожалуйста, удалите ключ безопасности и повторите попытку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="75"/>
        <source>Key Database Path Too Long</source>
        <translation>Путь к базе данных ключей слишком длинный</translation>
    </message>
    <message>
        <source>This folder&apos;s path is too long for GnuPG&apos;s agent socket, so GnuPG could not start against it. Choose a folder with a shorter path.</source>
        <translation type="vanished">Путь к этой папке слишком длинный для сокета агента GnuPG, поэтому GnuPG не удалось запустить. Выберите папку с более коротким путём.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="76"/>
        <source>This folder&apos;s path is too long for GnuPG&apos;s agent socket, and no short enough link to it could be made, so GnuPG could not start against it. Choose a folder with a shorter path.</source>
        <translation>Путь к этой папке слишком длинный для сокета агента GnuPG, и не удалось создать достаточно короткую ссылку на неё, поэтому GnuPG не удалось запустить. Выберите папку с более коротким путём.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="94"/>
        <source>Active</source>
        <translation>Активен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="95"/>
        <source>Inactive</source>
        <translation>Неактивен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="107"/>
        <source>GpgFrontend could not open this key database at the last start.</source>
        <translation>GpgFrontend не удалось открыть эту базу данных ключей при последнем запуске.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="123"/>
        <source>ID</source>
        <translation>ID</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="125"/>
        <source>Role</source>
        <translation>Роль</translation>
    </message>
    <message>
        <source>Channel</source>
        <translation type="vanished">Канал</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="44"/>
        <source>N/A</source>
        <translation>Н/Д</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="53"/>
        <source>The signature shows this package was not altered after it was built. It does not show who built it: the key travels inside the package.</source>
        <translation>Подпись показывает, что этот пакет не был изменён после сборки. Она не показывает, кто его собрал: ключ находится внутри пакета.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="69"/>
        <source>Verified by GpgFrontend</source>
        <translation>Проверено GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="71"/>
        <source>Identifier</source>
        <translation>Идентификатор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="75"/>
        <source>Version</source>
        <translation>Версия</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="78"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="83"/>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="92"/>
        <source>Origin</source>
        <translation>Источник</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="79"/>
        <source>Built into this application</source>
        <translation>Встроено в это приложение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="80"/>
        <source>Ships with GpgFrontend and is not loaded from disk.</source>
        <translation>Поставляется с GpgFrontend и не загружается с диска.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="84"/>
        <source>Signed package</source>
        <translation>Подписанный пакет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="86"/>
        <source>Package</source>
        <translation>Пакет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="93"/>
        <source>Unsigned library</source>
        <translation>Неподписанная библиотека</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="94"/>
        <source>Loaded from a loose library, which nothing vouches for. Released builds ship signed packages only.</source>
        <translation>Загружена из отдельной библиотеки, за которую никто не ручается. Релизные сборки поставляются только с подписанными пакетами.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="111"/>
        <source>SDK ABI</source>
        <translation>SDK ABI</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="118"/>
        <source>Built for</source>
        <translation>Собрано для</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="125"/>
        <source>Built against Qt</source>
        <translation>Собрано с Qt</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="129"/>
        <source>Capabilities</source>
        <translation>Возможности</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="135"/>
        <source>Digest</source>
        <translation>Дайджест</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="163"/>
        <source>Described by its package</source>
        <translation>Описывается своим пакетом</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="164"/>
        <source>Claimed by the module</source>
        <translation>Заявлено модулем</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="167"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="230"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="123"/>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="172"/>
        <source>Author</source>
        <translation>Автор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="177"/>
        <source>Description</source>
        <translation>Описание</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="124"/>
        <source>Engine</source>
        <translation>Движок</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="124"/>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="66"/>
        <source>%1  ·  %2% used  ·  %3</source>
        <translation>%1  ·  Использовано %2%  ·  %3</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePage.cpp" line="72"/>
        <source>%1  ·  %2</source>
        <translation>%1  ·  %2</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FileTreeView.cpp" line="53"/>
        <source>%1 item(s)</source>
        <translation>%1 элемент(ов)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="98"/>
        <source>Open Link</source>
        <translation>Открыть ссылку</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="99"/>
        <source>Could not open a web browser on this system.

The link has been copied to your clipboard:
%1</source>
        <translation>Не удалось открыть веб-браузер в этой системе.

Ссылка скопирована в буфер обмена:
%1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="127"/>
        <source>Star GpgFrontend on GitHub</source>
        <translation>Поставить звезду GpgFrontend на GitHub</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/help/AboutDialog.cpp" line="134"/>
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
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="43"/>
        <source>Strong</source>
        <translation>Сильный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="158"/>
        <source>Fully Valid</source>
        <translation>Полностью действительный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="160"/>
        <source>Valid (with Issues)</source>
        <translation>Действительный (с замечаниями)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="162"/>
        <source>Valid (Not Fully Trusted)</source>
        <translation>Действительный (неполное доверие)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="164"/>
        <source>Invalid</source>
        <translation>Недействительный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="166"/>
        <source>Key Missing</source>
        <translation>Ключ отсутствует</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="168"/>
        <source>Key Revoked</source>
        <translation>Ключ отозван</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="170"/>
        <source>Signature Expired</source>
        <translation>Срок действия подписи истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="172"/>
        <source>Key Expired</source>
        <translation>Срок действия ключа истёк</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="174"/>
        <source>Verification Error</source>
        <translation>Ошибка проверки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="176"/>
        <source>Key Not Authorized</source>
        <translation>Ключ не уполномочен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="178"/>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="47"/>
        <source>Unknown</source>
        <translation>Неизвестно</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="184"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="661"/>
        <source>Signer</source>
        <translation>Подписант</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="186"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="209"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="256"/>
        <source>Fingerprint</source>
        <translation>Отпечаток</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="56"/>
        <source>1 key</source>
        <translation>1 ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="56"/>
        <source>%1 keys</source>
        <translation>%1 ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="64"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="152"/>
        <source>Contents</source>
        <translation>Содержимое</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="66"/>
        <source>Location</source>
        <translation>Расположение</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="180"/>
        <source>There are no keys to export.</source>
        <translation>Нет ключей для экспорта.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="182"/>
        <source>Choose a PIN to protect the key file.</source>
        <translation>Выберите PIN-код для защиты файла ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="184"/>
        <source>Choose where to save the key file.</source>
        <translation>Выберите, куда сохранить файл ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="186"/>
        <source>Enter a package name. It cannot contain / \ : * ? &quot; &lt; &gt; | or end with a dot or a space.</source>
        <translation>Введите имя пакета. Оно не может содержать / \ : * ? &quot; &lt; &gt; | или заканчиваться точкой или пробелом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="190"/>
        <source>Choose where to save the key package.</source>
        <translation>Выберите, куда сохранить пакет ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="192"/>
        <source>This folder cannot be written to. Choose another.</source>
        <translation>В эту папку нельзя записать. Выберите другую.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="194"/>
        <source>The key package and the key file would be the same file. Choose a different name or location for one of them.</source>
        <translation>Пакет ключей и файл ключа будут одним и тем же файлом. Выберите другое имя или расположение для одного из них.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="205"/>
        <source>1 key selected</source>
        <translation>Выбран 1 ключ</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="206"/>
        <source>%1 keys selected</source>
        <translation>%1 ключей выбрано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="209"/>
        <source>%1, none with a private key.</source>
        <translation>%1, ни у одного нет закрытого ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="213"/>
        <source>%1, with its private key.</source>
        <translation>%1, с закрытым ключом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="214"/>
        <source>%1, all with a private key.</source>
        <translation>%1, все с закрытым ключом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="216"/>
        <source>%1, %2 with a private key.</source>
        <translation>%1, %2 с закрытым ключом.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="224"/>
        <source>None of the selected keys has a private key, so only public keys can be exported.</source>
        <translation>Ни у одного из выбранных ключей нет закрытого ключа, поэтому можно экспортировать только открытые ключи.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="230"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="189"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="212"/>
        <source>Key ID</source>
        <translation>Идентификатор ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="230"/>
        <source>Email</source>
        <translation>Электронная почта</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="231"/>
        <source>Exported</source>
        <translation>Экспортировано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="243"/>
        <source>Left out: no private key</source>
        <translation>Не включён: нет закрытого ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="246"/>
        <source>Public and private</source>
        <translation>Открытый и закрытый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="248"/>
        <source>Public</source>
        <translation>Открытый</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="266"/>
        <source>Private keys</source>
        <translation>Закрытые ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="268"/>
        <source>Not included</source>
        <translation>Не включено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="275"/>
        <source>Key file protected by your PIN</source>
        <translation>Файл ключа защищён вашим PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="276"/>
        <source>Not set up yet</source>
        <translation>Ещё не настроено</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="280"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="300"/>
        <source>Key package</source>
        <translation>Пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="281"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="288"/>
        <source>Not chosen yet</source>
        <translation>Ещё не выбран</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="286"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="303"/>
        <source>Key file</source>
        <translation>Файл ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="305"/>
        <source>Needed with the PIN to import the package.</source>
        <translation>Требуется вместе с PIN для импорта пакета.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="311"/>
        <source>The keys are encrypted with a random 256-character passphrase (XChaCha20-Poly1305). That passphrase is sealed with your PIN (Argon2id) and written to the key file. Importing needs the package, the key file and the PIN.</source>
        <translation>Ключи зашифрованы случайной парольной фразой из 256 символов (XChaCha20-Poly1305). Эта парольная фраза запечатана вашим PIN (Argon2id) и записана в файл ключа. Для импорта нужны пакет, файл ключа и PIN.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="196"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="216"/>
        <source>Algorithm</source>
        <translation>Алгоритм</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="199"/>
        <source>Signed</source>
        <translation>Подписано</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="207"/>
        <location filename="../../../../src/ui/function/InfoBoardReport.cpp" line="657"/>
        <source>Recipient</source>
        <translation>Получатель</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="215"/>
        <source>Primary Key Algorithm</source>
        <translation>Алгоритм первичного ключа</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="231"/>
        <source>Signature · %1</source>
        <translation>Подпись · %1</translation>
    </message>
    <message>
        <source>Input Material Hash</source>
        <translation type="vanished">Хеш входных данных</translation>
    </message>
    <message>
        <source>SHA-256</source>
        <translation type="vanished">SHA-256</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="244"/>
        <source>Signature Created</source>
        <translation>Подпись создана</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="247"/>
        <source>Mode</source>
        <translation>Режим</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="254"/>
        <source>Invalid Signer</source>
        <translation>Недействительный подписант</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="258"/>
        <source>Reason</source>
        <translation>Причина</translation>
    </message>
    <message>
        <source>Decrypt</source>
        <translation type="vanished">Расшифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="273"/>
        <source>Message Metadata</source>
        <translation>Метаданные сообщения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="140"/>
        <source>Open a Profile File</source>
        <translation>Открыть файл профиля</translation>
    </message>
    <message>
        <source>This file is protected by a passphrase. Enter it to read what is inside.</source>
        <translation type="vanished">Этот файл защищён парольной фразой. Введите её, чтобы прочитать содержимое.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="143"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="224"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="661"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="673"/>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <source>Nothing in this file can be read until the passphrase opens it.</source>
        <translation type="vanished">Ничего в этом файле не может быть прочитано, пока парольная фраза не откроет его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="154"/>
        <source>Protect This Profile File</source>
        <translation>Защитить этот файл профиля</translation>
    </message>
    <message>
        <source>This is the passphrase the file will be sealed with from now on. If you enter a different one, the old one will no longer open it.</source>
        <translation type="vanished">Это парольная фраза, которой файл будет запечатан отныне. Если вы введёте другую, старая больше не откроет его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="156"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="157"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="201"/>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <source>Keep this passphrase. The file cannot be opened without it.</source>
        <translation type="vanished">Сохраните эту парольную фразу. Файл не может быть открыт без неё.</translation>
    </message>
    <message>
        <source>If you lose this passphrase, this file can never be opened again. There is no recovery.</source>
        <translation type="vanished">Если вы потеряете эту парольную фразу, этот файл никогда не сможет быть открыт снова. Восстановление невозможно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/controller/ModuleMeta.cpp" line="100"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="243"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="54"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="120"/>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="276"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="246"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="57"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="123"/>
        <location filename="../../../../src/ui/dialog/settings/SettingsKeyDatabases.cpp" line="125"/>
        <source>Folder</source>
        <translation>Папка</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="213"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="251"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="65"/>
        <source>Size</source>
        <translation>Размер</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="66"/>
        <source>Modified</source>
        <translation>Изменён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="68"/>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="274"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="154"/>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="223"/>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="80"/>
        <source>Protection</source>
        <translation>Защита</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="81"/>
        <source>Not sealed</source>
        <translation>Не запечатан</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="82"/>
        <source>Anyone holding this file can read what is in it.</source>
        <translation>Любой, у кого есть этот файл, может прочитать его содержимое.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="89"/>
        <source>Created</source>
        <translation>Создан</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="96"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="190"/>
        <source>Written by</source>
        <translation>Автор</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="98"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="191"/>
        <source>GpgFrontend %1</source>
        <translation>GpgFrontend %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="99"/>
        <source>GpgFrontend %1 (development build)</source>
        <translation>GpgFrontend %1 (сборка для разработки)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="109"/>
        <source>From the file&apos;s header (unverified)</source>
        <translation>Из заголовка файла (не проверено)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfilePackageMeta.cpp" line="133"/>
        <source>Free space</source>
        <translation>Свободное место</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="142"/>
        <source>Enter the passphrase that opens this file.</source>
        <translation>Введите парольную фразу, которая открывает этот файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="156"/>
        <source>Choose the passphrase this file will be sealed with.</source>
        <translation>Выберите парольную фразу, которой будет запечатан этот файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="161"/>
        <source>A different passphrase replaces the old one, which will no longer open this file.</source>
        <translation>Другая парольная фраза заменяет прежнюю; прежняя больше не откроет этот файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="164"/>
        <source>If you lose this passphrase, this file can never be opened again.</source>
        <translation>Если вы потеряете эту парольную фразу, этот файл никогда не сможет быть открыт снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="169"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="170"/>
        <source>Passphrase</source>
        <translation>Парольная фраза</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="171"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="210"/>
        <source>Repeat</source>
        <translation>Повторите</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="172"/>
        <source>Show passphrase</source>
        <translation>Показать парольную фразу</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="173"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="212"/>
        <source>Strength</source>
        <translation>Надёжность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="176"/>
        <source>The passphrase must be at least %1 characters.</source>
        <translation>Парольная фраза должна быть длиной не менее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="177"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="217"/>
        <source>The two entries do not match.</source>
        <translation>Введённые значения не совпадают.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="194"/>
        <source>Import Key Package</source>
        <translation>Импортировать пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="195"/>
        <source>Enter the PIN that opens this key file.</source>
        <translation>Введите PIN, который открывает этот файл ключа.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="198"/>
        <source>Protect This Key Package</source>
        <translation>Запечатать этот пакет ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="200"/>
        <source>Choose the PIN that will seal the package&apos;s key file.</source>
        <translation>Выберите PIN, которым будет запечатан файл ключа пакета.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="203"/>
        <source>Use at least %1 characters.</source>
        <translation>Используйте не менее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="204"/>
        <source>If you lose this PIN, the key package can never be opened again.</source>
        <translation>Если вы потеряете этот PIN, пакет ключей больше никогда не удастся открыть.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="208"/>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="209"/>
        <source>PIN</source>
        <translation>PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="211"/>
        <source>Show PIN</source>
        <translation>Показать PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="213"/>
        <source>Generate a strong PIN</source>
        <translation>Сгенерировать надёжный PIN</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="216"/>
        <source>The PIN must be at least %1 characters.</source>
        <translation>PIN должен содержать не менее %1 символов.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="277"/>
        <source>Cipher</source>
        <translation>Шифр</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="278"/>
        <source>MIME</source>
        <translation>MIME</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="279"/>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="280"/>
        <source>No</source>
        <translation>Нет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="281"/>
        <source>Integrity</source>
        <translation>Целостность</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="283"/>
        <source>Protected</source>
        <translation>Защищён</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="284"/>
        <source>Not Protected (unsafe)</source>
        <translation>Не защищён (небезопасно)</translation>
    </message>
    <message>
        <source>Encrypt</source>
        <translation type="vanished">Зашифровать</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="292"/>
        <source>Encryption Recipient</source>
        <translation>Получатель шифрования</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/InfoBoardCardConverter.cpp" line="303"/>
        <source>Decryption Recipient</source>
        <translation>Получатель расшифровки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="195"/>
        <source>ECC</source>
        <translation>ECC</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyGenerateHelper.cpp" line="202"/>
        <source>Post-Quantum</source>
        <translation>Постквантовый</translation>
    </message>
    <message>
        <location filename="../../../../src/core/profile/ProfileSecureKeyManager.cpp" line="550"/>
        <source>The secure key could not be encrypted, so it was not saved to disk.</source>
        <translation>Защищённый ключ не удалось зашифровать, поэтому он не был сохранён на диск.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="39"/>
        <source>Standard</source>
        <translation>Стандартный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="41"/>
        <source>Enhanced</source>
        <translation>Расширенный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="45"/>
        <source>Maximum</source>
        <translation>Максимальный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="54"/>
        <source>System keychain</source>
        <translation>Системная связка ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="56"/>
        <source>PIN at startup</source>
        <translation>PIN при запуске</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/SecurityDisplayNames.cpp" line="60"/>
        <source>No extra protection</source>
        <translation>Без дополнительной защиты</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/FilePathBar.cpp" line="90"/>
        <source>Home</source>
        <translation>Главная</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="96"/>
        <source>The new window could not be started.</source>
        <translation>Не удалось открыть новое окно.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="104"/>
        <source>Name This Profile</source>
        <translation>Назвать этот профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="105"/>
        <source>What should this profile be called on this computer?</source>
        <translation>Как назвать этот профиль на этом компьютере?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="129"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="149"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="167"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="176"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="705"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="727"/>
        <source>Cannot Import Profile</source>
        <translation>Не удалось импортировать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="150"/>
        <source>This profile was made by a newer version of GpgFrontend (%1).</source>
        <translation>Этот профиль создан более новой версией GpgFrontend (%1).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="211"/>
        <location filename="../../../../src/ui/function/GuiProfileLoaderDelegate.cpp" line="174"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="188"/>
        <source>Profile</source>
        <translation>Профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="194"/>
        <source>Workspace files</source>
        <translation>Файлы рабочей области</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="267"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="196"/>
        <source>Included</source>
        <translation>Включены</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="197"/>
        <source>None in the file</source>
        <translation>В файле ничего нет</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/import_export/KeyPackageExportSummary.cpp" line="263"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="206"/>
        <source>Keys</source>
        <translation>Ключи</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="209"/>
        <source>&quot;%1&quot; is unavailable</source>
        <translation>&quot;%1&quot; недоступен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="211"/>
        <source>It pointed at keys kept outside the profile, which do not travel. Point it somewhere on this computer to use it.</source>
        <translation>Он указывал на ключи, хранящиеся вне профиля, которые не переносятся. Укажите на него где-нибудь на этом компьютере, чтобы использовать его.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="219"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="786"/>
        <source>&quot;%1&quot; is ready.</source>
        <translation>&quot;%1&quot; готов.</translation>
    </message>
    <message>
        <source>The file did not carry any workspace files.</source>
        <translation type="vanished">Файл не содержал файлов рабочей области.</translation>
    </message>
    <message>
        <source>&quot;%1&quot; pointed at keys kept outside the profile, which do not travel. It will show as unavailable until you point it somewhere on this computer.</source>
        <translation type="vanished">&quot;%1&quot; указывал на ключи, хранящиеся вне профиля, которые не переносятся. Он будет отображаться как недоступный, пока вы не укажете на него где-то на этом компьютере.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="218"/>
        <source>Profile Imported</source>
        <translation>Профиль импортирован</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="221"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="787"/>
        <source>Open it now? It opens in a new window.</source>
        <translation>Открыть его сейчас? Он откроется в новом окне.</translation>
    </message>
    <message>
        <source>made %1</source>
        <translation type="vanished">создан %1</translation>
    </message>
    <message>
        <source>by GpgFrontend %1</source>
        <translation type="vanished">программой GpgFrontend %1</translation>
    </message>
    <message>
        <source>by a development build of GpgFrontend %1</source>
        <translation type="vanished">разработческой сборкой GpgFrontend %1</translation>
    </message>
    <message>
        <source>The file says it was %1.</source>
        <translation type="vanished">Файл сообщает, что он был %1.</translation>
    </message>
    <message>
        <source>That comes from the file&apos;s unencrypted header, which anyone holding the file can change. Nothing is confirmed until the passphrase opens it.</source>
        <translation type="vanished">Это берётся из незашифрованного заголовка файла, который может изменить любой, у кого есть файл. Ничто не подтверждается, пока файл не будет открыт парольной фразой.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="225"/>
        <source>Not Now</source>
        <translation>Не сейчас</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="272"/>
        <source>GpgFrontend Profile File</source>
        <translation>Файл профиля GpgFrontend</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="95"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="347"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="364"/>
        <source>Default</source>
        <translation>По умолчанию</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="349"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="366"/>
        <source>Portable</source>
        <translation>Портативный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="351"/>
        <source>From a package</source>
        <translation>Из пакета</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="353"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="368"/>
        <source>Temporary</source>
        <translation>Временный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="355"/>
        <source>Local</source>
        <translation>Локальный</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="410"/>
        <source>This file is no longer there:</source>
        <translation>Этого файла больше нет:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="419"/>
        <source>This window is already using that profile.</source>
        <translation>Это окно уже использует этот профиль.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="429"/>
        <source>There is no profile called &quot;%1&quot;.</source>
        <translation>Нет профиля с именем «%1».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="448"/>
        <source>&quot;%1&quot; is open in another window (process %2 on %3).</source>
        <translation>«%1» открыт в другом окне (процесс %2 на %3).</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="453"/>
        <source>&quot;%1&quot; is open in another window.</source>
        <translation>«%1» открыт в другом окне.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="485"/>
        <source>Save Changes?</source>
        <translation>Сохранить изменения?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="486"/>
        <source>This profile was opened from a file. It is not kept on this computer, and the copy it is running from is about to be deleted.</source>
        <translation>Этот профиль был открыт из файла. Он не хранится на этом компьютере, и копия, из которой он запущен, будет удалена.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="491"/>
        <source>Anything you changed is lost unless it is written back into that file.</source>
        <translation>Любые изменения будут потеряны, если их не записать обратно в этот файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="496"/>
        <source>Save Changes</source>
        <translation>Сохранить изменения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="498"/>
        <source>Discard</source>
        <translation>Отбросить</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="499"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="676"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="813"/>
        <source>Destroy Temporary Profile?</source>
        <translation>Уничтожить временный профиль?</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="815"/>
        <source>Everything in this temporary profile will be destroyed: its keys, settings and files.</source>
        <translation>Всё в этом временном профиле будет уничтожено: его ключи, настройки и файлы.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="818"/>
        <source>To keep it, choose Cancel and use Profiles &gt; Export This Profile first.</source>
        <translation>Чтобы сохранить его, выберите «Отмена» и сначала воспользуйтесь «Профили &gt; Экспортировать этот профиль».</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="821"/>
        <source>Destroy and Close</source>
        <translation>Уничтожить и закрыть</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="836"/>
        <source>Restart Not Possible</source>
        <translation>Перезапуск невозможен</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="838"/>
        <source>These changes need GpgFrontend to restart, and restarting would destroy this temporary profile.</source>
        <translation>Эти изменения требуют перезапуска GpgFrontend, а перезапуск уничтожит этот временный профиль.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="842"/>
        <source>They can&apos;t be applied in a temporary profile.</source>
        <translation>Их нельзя применить во временном профиле.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="843"/>
        <source>Pages with changes that need this: %1.

Undo those changes, or choose Cancel to discard everything you changed in this dialog.</source>
        <translation>Страницы с изменениями, требующими этого: %1.

Отмените эти изменения или выберите «Отмена», чтобы отменить всё, что вы изменили в этом диалоге.</translation>
    </message>
    <message>
        <source>Enter the passphrase to protect this file with:</source>
        <translation type="vanished">Введите парольную фразу для защиты этого файла:</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="585"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="607"/>
        <source>Cannot Save Changes</source>
        <translation>Не удалось сохранить изменения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="586"/>
        <source>The application key is not available, so the profile could not be packed.</source>
        <translation>Ключ приложения недоступен, поэтому профиль не удалось упаковать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="594"/>
        <source>Saving Profile</source>
        <translation>Сохранение профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="650"/>
        <source>&quot;%1&quot; is a profile file. Choose what to do with it.</source>
        <translation>&quot;%1&quot; — это файл профиля. Выберите, что с ним делать.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="660"/>
        <source>What happens next</source>
        <translation>Что будет дальше</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="662"/>
        <source>Work in the file itself</source>
        <translation>Работать в самом файле</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="663"/>
        <source>Nothing is kept on this computer. What you change is written back into the file.</source>
        <translation>Ничего не сохраняется на этом компьютере. Изменения записываются обратно в файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="667"/>
        <source>Copy it into a profile kept here</source>
        <translation>Скопировать в профиль, который хранится здесь</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="668"/>
        <source>The copy lives on this computer from then on, and the file is not used again.</source>
        <translation>Копия остаётся на этом компьютере, и файл больше не используется.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="688"/>
        <source>Import Profile File</source>
        <translation>Импорт файла профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="728"/>
        <source>A temporary folder could not be made.</source>
        <translation>Не удалось создать временную папку.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="735"/>
        <source>Reading Profile</source>
        <translation>Чтение профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="759"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="775"/>
        <source>Cannot Create Profile</source>
        <translation>Не удалось создать профиль</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="168"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="760"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="776"/>
        <source>The profile could not be created.</source>
        <translation>Профиль не удалось создать.</translation>
    </message>
    <message>
        <source>It is sealed with a passphrase, which is asked for once you choose.</source>
        <translation type="vanished">Он запечатан парольной фразой; её запросят после того, как вы выберете.</translation>
    </message>
    <message>
        <source>It is not sealed with a passphrase: anyone holding it can read what is in it.</source>
        <translation type="vanished">Он не запечатан парольной фразой: любой, у кого он есть, может прочитать, что в нём.</translation>
    </message>
    <message>
        <source>Open it to work in it directly and leave it a file, with nothing kept on this computer. Import it to copy it into a profile kept here, after which the file is not used again.</source>
        <translation type="vanished">Откройте его, чтобы работать с ним напрямую и оставить его в виде файла, ничего не сохраняя на этом компьютере. Импортируйте его, чтобы скопировать его в профиль, хранящийся здесь; после этого файл больше не используется.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="649"/>
        <source>Open Profile File</source>
        <translation>Открыть файл профиля</translation>
    </message>
    <message>
        <source>&quot;%1&quot; is a profile file.</source>
        <translation type="vanished">«%1» — файл профиля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/SecretPrompt.cpp" line="196"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="666"/>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="675"/>
        <source>Import</source>
        <translation>Импорт</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/ProfileController.cpp" line="785"/>
        <source>Profile Created</source>
        <translation>Профиль создан</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/AppKeyPinDialog.cpp" line="158"/>
        <source>That passphrase did not open this file. Try again.</source>
        <translation>Эта парольная фраза не открыла файл. Попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="46"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="50"/>
        <source>Saved state, key groups and categories</source>
        <translation>Сохранённое состояние, группы ключей и категории</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="58"/>
        <source>This profile&apos;s own key</source>
        <translation>Собственный ключ этого профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="62"/>
        <source>Keys stored inside this profile</source>
        <translation>Ключи, хранящиеся внутри этого профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="65"/>
        <source>My workspace files</source>
        <translation>Мои рабочие файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="143"/>
        <source>Encrypted with XChaCha20-Poly1305, using a key derived from your passphrase with Argon2id. The system keychain is not used, so the file opens on any computer.</source>
        <translation>Зашифровано с помощью XChaCha20-Poly1305, ключ получен из вашей парольной фразы с помощью Argon2id. Системная связка ключей не используется, поэтому файл открывается на любом компьютере.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="181"/>
        <source>Settings, keys, this profile&apos;s own key and workspace files, %1 before compression.</source>
        <translation>Настройки, ключи, собственный ключ этого профиля и рабочие файлы, %1 до сжатия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="183"/>
        <source>Settings, keys and this profile&apos;s own key, %1 before compression.</source>
        <translation>Настройки, ключи и собственный ключ этого профиля, %1 до сжатия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="200"/>
        <source>Include workspace files (%1)</source>
        <translation>Включить рабочие файлы (%1)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="203"/>
        <source>No workspace files to include</source>
        <translation>Нет рабочих файлов для включения</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="215"/>
        <source>%1, including workspace files</source>
        <translation>%1, включая рабочие файлы</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="218"/>
        <source>Before compression.</source>
        <translation>До сжатия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="225"/>
        <source>Encrypted with your passphrase</source>
        <translation>Зашифровано вашей парольной фразой</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="226"/>
        <source>No passphrase chosen yet</source>
        <translation>Парольная фраза ещё не выбрана</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="236"/>
        <source>%1 free on this drive</source>
        <translation>%1 свободно на этом диске</translation>
    </message>
    <message>
        <source>Writes %1: keys, settings and workspace files, about %2 before compression.</source>
        <translation type="vanished">Записывает %1: ключи, настройки и рабочие файлы, примерно %2 до сжатия.</translation>
    </message>
    <message>
        <source>Writes %1: keys and settings, about %2 before compression.</source>
        <translation type="vanished">Записывает %1: ключи и настройки, примерно %2 до сжатия.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="124"/>
        <source>Total</source>
        <translation>Итого</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="133"/>
        <source>Never included: logs, modules, and keys kept elsewhere.</source>
        <translation>Никогда не включаются: журналы, модули и ключи, хранящиеся в другом месте.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="135"/>
        <source>Keys kept outside this profile, such as the system GnuPG keyring, stay where they are.</source>
        <translation>Ключи, хранящиеся вне этого профиля, например системная связка ключей GnuPG, остаются на своих местах.</translation>
    </message>
    <message>
        <source>Key derivation</source>
        <translation type="vanished">Формирование ключа</translation>
    </message>
    <message>
        <source>Argon2id, from your passphrase</source>
        <translation type="vanished">Argon2id, из вашей парольной фразы</translation>
    </message>
    <message>
        <source>Keychain</source>
        <translation type="vanished">Связка ключей</translation>
    </message>
    <message>
        <source>Not used: the file has to open on another computer</source>
        <translation type="vanished">Не используется: файл должен открываться на другом компьютере.</translation>
    </message>
    <message>
        <source>Anyone who gets this file can read your keys and everything in the profile, and can change it before you import it.</source>
        <translation type="vanished">Любой, кто получит этот файл, сможет прочитать ваши ключи и всё содержимое профиля, а также изменить его до того, как вы его импортируете.</translation>
    </message>
    <message>
        <source>Anyone who gets this file can read your keys, everything in the profile, and your workspace files — including anything you meant to encrypt but had not yet.</source>
        <translation type="vanished">Любой, кто получит этот файл, сможет прочитать ваши ключи, всё содержимое профиля и ваши рабочие файлы — включая то, что вы собирались зашифровать, но ещё не сделали этого.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/dialog/profile/ProfileExportSummary.cpp" line="101"/>
        <source>There may not be room for this where you are saving it. The file is compressed as it is written, so it may still fit.</source>
        <translation>Возможно, в месте сохранения недостаточно места. Файл сжимается при записи, поэтому он всё ещё может поместиться.</translation>
    </message>
    <message>
        <source>A passphrase-protected file holding your keys, settings and workspace files — about %1 before compression — will be written to %2.</source>
        <translation type="vanished">Защищённый парольной фразой файл, содержащий ваши ключи, настройки и рабочие файлы — около %1 до сжатия — будет записан в %2.</translation>
    </message>
    <message>
        <source>A passphrase-protected file holding your keys and settings — about %1 before compression — will be written to %2.</source>
        <translation type="vanished">Защищённый парольной фразой файл, содержащий ваши ключи и настройки — около %1 до сжатия — будет записан в %2.</translation>
    </message>
    <message>
        <source>An unprotected file holding your keys, settings and workspace files — about %1 before compression — will be written to %2.</source>
        <translation type="vanished">Незащищенный файл, содержащий ваши ключи, настройки и файлы рабочей области — около %1 до сжатия — будет записан в %2.</translation>
    </message>
    <message>
        <source>An unprotected file holding your keys and settings — about %1 before compression — will be written to %2.</source>
        <translation type="vanished">Незащищенный файл, содержащий ваши ключи и настройки — около %1 до сжатия — будет записан в %2.</translation>
    </message>
    <message>
        <source>This profile&apos;s own default key database</source>
        <translation type="vanished">Собственная база данных ключей по умолчанию этого профиля</translation>
    </message>
    <message>
        <source>This computer&apos;s default key database</source>
        <translation type="vanished">База данных ключей по умолчанию этого компьютера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="45"/>
        <source>This profile&apos;s own built-in key database</source>
        <translation>Собственная встроенная база данных ключей этого профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="46"/>
        <source>This computer&apos;s built-in key database</source>
        <translation>Встроенная база данных ключей этого компьютера</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="48"/>
        <source>Kept inside your profile</source>
        <translation>Хранится внутри вашего профиля</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="50"/>
        <source>On this computer only</source>
        <translation>Только на этом компьютере</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="86"/>
        <source>Uses keys in the GnuPG home directory.</source>
        <translation>Использует ключи в домашнем каталоге GnuPG.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseDisplayNames.cpp" line="87"/>
        <source>Uses the rPGP key database at this location.</source>
        <translation>Использует базу данных ключей rPGP по этому пути.</translation>
    </message>
    <message>
        <source>Channel %1</source>
        <translation type="vanished">Канал %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/MetaListPanel.cpp" line="81"/>
        <source>These come from the file&apos;s unencrypted header, which anyone holding the file can change.</source>
        <translation>Эти данные берутся из незашифрованного заголовка файла, который может изменить любой, у кого есть файл.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/ModuleManager.cpp" line="173"/>
        <source>The module claims the ID %1, which belongs to a module that comes with GpgFrontend.</source>
        <translation>Модуль заявляет ID %1, который принадлежит модулю, поставляемому с GpgFrontend.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/ModuleManager.cpp" line="204"/>
        <source>External modules are not supported on macOS: the system only loads code signed with this application&apos;s own Team ID.</source>
        <translation>Внешние модули не поддерживаются в macOS: система загружает только код, подписанный собственным Team ID этого приложения.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/ModuleManager.cpp" line="219"/>
        <source>Waiting for you to trust the publisher key that signed it.</source>
        <translation>Ожидается, что вы доверитесь ключу издателя, подписавшему его.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/ModuleManager.cpp" line="222"/>
        <source>Waiting for you to enable it.</source>
        <translation>Ожидается, что вы включите его.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/ModuleManager.cpp" line="249"/>
        <source>The module declares the ID %1 but is in a directory named %2 instead of %3.</source>
        <translation>Модуль объявляет ID %1, но находится в каталоге %2, а не %3.</translation>
    </message>
    <message>
        <location filename="../../../../src/core/module/ModuleManager.cpp" line="323"/>
        <source>This is not a module package.</source>
        <translation>Это не пакет модуля.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="202"/>
        <source>Key databases updated.</source>
        <translation>Базы данных ключей обновлены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="208"/>
        <source>These key databases could not be opened: %1</source>
        <translation>Не удалось открыть эти базы данных ключей: %1</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="212"/>
        <source>The key databases in use are unchanged.</source>
        <translation>Используемые базы данных ключей не изменены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="216"/>
        <source>Key databases updated, with problems.</source>
        <translation>Базы данных ключей обновлены с проблемами.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="217"/>
        <source>Key database changes were not applied.</source>
        <translation>Изменения базы данных ключей не применены.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="220"/>
        <source>Key Database Changes</source>
        <translation>Изменения базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="253"/>
        <source>%1 (key database no longer available)</source>
        <translation>%1 (база данных ключей больше недоступна)</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="281"/>
        <source>Applying key database changes...</source>
        <translation>Применение изменений базы данных ключей...</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/function/KeyDatabaseRefresh.cpp" line="289"/>
        <source>Applying Key Database Changes</source>
        <translation>Применение изменений базы данных ключей</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="87"/>
        <source>Default: used when an operation does not explicitly select a key database.</source>
        <translation>По умолчанию: используется, если в операции явно не выбрана база данных ключей.</translation>
    </message>
    <message>
        <location filename="../../../../src/ui/widgets/KeyList.cpp" line="109"/>
        <source>ID %1</source>
        <translation>ID %1</translation>
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
