/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.0.5-rc4
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7bstrm.hpp
 * Content size  : 3510
 * Date / Time   : 17-09-2026 01:09:41
 * MD5           : fe97a4ea062b7efa7f9a574deb492038
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhlibrle7binc.hpp"

class VHRLE7bStreams
{
public:
    // Callback function template for input data
    typedef bool (*CallbackFunc_VHLIBRLE7B_IDATA)(uint8_t *pbv, size_t rpos);

    // Callback function template for output data
    typedef bool (*CallbackFunc_VHLIBRLE7B_ODATA)(uint8_t bv, size_t wpos, void *phdr);

    // Single-byte API: both funcs should be valid for sbyte mode

    /**
     * @brief Конструктор инициализации потоков
     * @param rbyte Callback функция для ввода данных
     * @param wbyte Callback функция для вывода данных
     * @return void - инициализация потоков
     */
    VHRLE7bStreams(
        CallbackFunc_VHLIBRLE7B_IDATA rbyte,
        CallbackFunc_VHLIBRLE7B_ODATA wbyte)
    {
        getbyte = rbyte;
        putbyte = wbyte;
        SetRStreamPos(0);
        SetWStreamPos(0);
    }

    /**
     * @brief Проверяет, инициализированы ли потоки корректно
     * @return bool - true если все callback функции инициализированы
     */
    bool isInitialized()
    {
        return checkCallbacks(getbyte, putbyte);
    }

    /**
     * @brief Читает один байт из потока
     * @param databyte Указатель на буфер для вывода данных
     * @return bool - true при успешном чтении
     */
    bool readbyte(uint8_t *databyte) { return getbyte(databyte, rpos++); }

    /**
     * @brief Записывает один байт в поток
     * @param databyte Данные для записи
     * @param phdr Дополнительные данные
     * @return bool - true при успешной записи
     */
    bool writebyte(uint8_t databyte, void *phdr) { return putbyte(databyte, wpos++, phdr); }

    /**
     * @brief Устанавливает позицию ввода
     * @param pos Позиция чтения
     * @return void - установка позиции
     */
    void SetRStreamPos(size_t pos) { rpos = pos; }

    /**
     * @brief Устанавливает позицию вывода
     * @param pos Позиция записи
     * @return void - установка позиции
     */

    void SetWStreamPos(size_t pos) { wpos = pos; }

    /**
     * @brief Возвращает позицию ввода
     * @return size_t - текущая позиция чтения
     */

    size_t GetRStreamPos() { return rpos; }

    /**
     * @brief Возвращает позицию вывода
     * @return size_t - текущая позиция записи
     */
    size_t GetWStreamPos() { return wpos; }

private:
    //
    CallbackFunc_VHLIBRLE7B_IDATA getbyte;
    CallbackFunc_VHLIBRLE7B_ODATA putbyte;

    size_t rpos;
    size_t wpos;

    /**
     * @brief Проверяет корректность указателей на функции
     * @param funcIn Указатель на функцию ввода
     * @param funcOut Указатель на функцию вывода
     * @return bool - true если функции валидны
     */
    bool checkCallbacks(
        CallbackFunc_VHLIBRLE7B_IDATA funcIn,
        CallbackFunc_VHLIBRLE7B_ODATA funcOut)
    {
        if (funcIn == nullptr)
            return false;
        if (funcOut == nullptr)
            return false;
        return true;
    }
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7bstrm.hpp
 * Revision         : 0.0.5-rc4
 * Content size     : 3510
 * Date / Time      : 17-09-2026 01:09:41
 * MD5              : fe97a4ea062b7efa7f9a574deb492038
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */