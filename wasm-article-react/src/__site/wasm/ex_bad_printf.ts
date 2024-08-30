import { WASM } from '../index.js';


const flag_left_align = 0x01;
const flag_plus_sign = 0x02;
const flag_plus_space = 0x04;
const flag_zero_pad = 0x08;
const flag_hash_modifier = 0x20;
const ixx:any = [
	null, 
	Int8Array, //1
	Int16Array, //2
	null, Int32Array, //4 
	null, null, null, BigInt64Array //8 
];
const uxx:any = [
	null, 
	Uint8Array, //1
	Uint16Array, //2
	null, Uint32Array, //4 
	null, null, null, BigUint64Array //8 
];

let printf_buffer = '';


function handle_newline_and_update(string:string) {
    let count = 0, length = string.length;
    for (let si = string.length - 1; si >= 0; --si) {
        if (string[si] == '\n') {
            --si, ++count;
            if (si >= 0 && string[si] == '\r') --si, ++count;
        } else if (string[si] == '\r') {
            --si, ++count;
        }
        if (count > 0) {
            ++si;
            printf_buffer += string.substring(0, si);
            console.log('[wasm]', printf_buffer);
            printf_buffer = '';
            string = string.substring(si + count);
            break;
        }
    }
    printf_buffer += string;
    return length;
}
function add_padding_to_number(prefix:string, number:string, padding_length:number, flags:number) {
    let buffer_length = printf_buffer.length;
    if (padding_length > 0) {
        let pad = ((flags & flag_zero_pad) ? '0' : ' ');
        let padding = ''; while (padding_length-- > 0) padding += pad;
        printf_buffer += (
            pad == '0' ? prefix + padding + number :
                (flags & flag_left_align)
                    ? prefix + number + padding : padding + prefix + number
        );
    } else {
        printf_buffer += prefix + number;
    }

    return printf_buffer.length - buffer_length;
}



export const printf_imports = Object.freeze({
    peek() {
        console.log(printf_buffer);
    },
    flush() {
        console.log(printf_buffer);
        printf_buffer = '';
    },
    printf_flush() {
        this.flush();
    },
    putchar(value:number) {
        let string = String.fromCharCode(value);
        return handle_newline_and_update(string);
    },
	puts: function(pointer:number) {

		let si = pointer;
		let view = WASM.ui8_rawmem();
		let max_si = view.byteLength;
		while (view[si] != 0 && si < max_si) ++si;
		console.log(printf_buffer+WASM.string(pointer, si - pointer));
		printf_buffer = '';

		return 0;
	},
    printf_string(begin:number, end:number) {
        let string = WASM.string(begin, end - begin);
        return handle_newline_and_update(string);
    },
    printf_integer(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {

        if (precision == 0 && value == 0) return 0;
        let absolute = (value < 0 ? -value : value);
        let number = '';

        if (!ixx[length]) { console.log(ixx, length); this.flush(); }
        number = (new ixx[length]([absolute]))[0].toString();
        let sign = (
            value < 0 ? '-' :
                value == 0 ? '' :
                    flags & flag_plus_sign ? '+' :
                        flags & flag_plus_space ? ' ' : ''
        );
        let prec_padding_length = precision - number.length;
        while (prec_padding_length-- > 0) number = '0' + number;
        if (precision >= 0) flags &= ~flag_zero_pad;
        let padding_length = width - number.length - sign.length;
        return add_padding_to_number(sign, number, padding_length, flags);
    },
    printf_int32(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_integer(flags, width, precision, length_char, length, type, value);
    },
    printf_int64(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_integer(flags, width, precision, length_char, length, type, value);
    },
    printf_long(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_integer(flags, width, precision, length_char, length, type, value);
    },
    printf_long_long(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_integer(flags, width, precision, length_char, length, type, value);
    },
    printf_intmax(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_integer(flags, width, precision, length_char, length, type, value);
    },
    printf_signed_size(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_integer(flags, width, precision, length_char, length, type, value);
    },
    printf_ptrdiff(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_integer(flags, width, precision, length_char, length, type, value);
    },

    printf_unsigned(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        if (precision == 0 && value == 0) return 0;
        let c = String.fromCharCode(type);
        let lc = String.fromCharCode(type | 0x20); // toLowerCase
        let base = (lc == 'o' ? 8 : lc == 'u' ? 10 : 16);
        let number = (new (uxx[length])([value]))[0].toString(base);
        if (c == 'X') number = number.toUpperCase();
        let prefix = '';
        let prec_padding_length = precision - number.length;
        if ((flags & flag_hash_modifier) && lc != 'u') {
            prefix = (lc == 'o' ? (prec_padding_length > 0 ? '' : '0') : '0' + c);
        }
        while (prec_padding_length-- > 0) number = '0' + number;
        if (precision >= 0) flags &= ~flag_zero_pad;
        let padding_length = width - number.length - prefix.length;
        return add_padding_to_number(prefix, number, padding_length, flags);
    },
    printf_uint32(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_unsigned(flags, width, precision, length_char, length, type, value);
    },
    printf_uint64(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_unsigned(flags, width, precision, length_char, length, type, value);
    },
    printf_unsigned_long(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_unsigned(flags, width, precision, length_char, length, type, value);
    },
    printf_unsigned_long_long(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_unsigned(flags, width, precision, length_char, length, type, value);
    },
    printf_uintmax(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_unsigned(flags, width, precision, length_char, length, type, value);
    },
    printf_size(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_unsigned(flags, width, precision, length_char, length, type, value);
    },
    printf_unsigned_ptrdiff(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        return this.printf_unsigned(flags, width, precision, length_char, length, type, value);
    },

    printf_double(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        let c = String.fromCharCode(type);
        let lc = String.fromCharCode(type | 0x20);
        if (lc == 'a') {
            printf_buffer += '(%a, %A: UNSUPPORTED)';
            return;
        } else if (length_char == 'L'.charCodeAt(0)) {
            printf_buffer += "(long double: UNSUPPORTED)";
            return;
        }
        let absolute = (value < 0 ? -value : value);
        let sign = (
            value < 0 ? '-' :
                value == 0 ? (1 / value < 0 ? '-' : '') :
                    flags & flag_plus_sign ? '+' :
                        flags & flag_plus_space ? ' ' : ''
        );
        if (absolute === Infinity || isNaN(value)) {
            let number = (absolute === Infinity
                ? (lc == c ? 'inf' : 'INF') : (lc == c ? 'nan' : 'NAN')
            );
            let padding_length = width - sign.length - number.length;
            return add_padding_to_number(sign, number, padding_length, (flags & ~flag_zero_pad));
        }
        if (lc == 'f') {
            let number = absolute.toFixed(precision == -1 ? 6 : precision);
            if (precision == 0 && (flags & flag_hash_modifier)) number += '.';
            let padding_length = width - sign.length - number.length;
            return add_padding_to_number(sign, number, padding_length, flags);
        } else {
            let number = absolute.toString();
            // decompose the numbers to its parts
            let di, msd:any = '0', mantissa = '', exponent_adjust = 0;
            let exponent:any = 0, exponent_sign:any = 1;
            for (di = 0; di < number.length; ++di) {
                if (number[di] == '.') break;
                if (msd == '0') { msd = number[di]; }
                else { ++exponent_adjust; mantissa += number[di]; }
            }
            for (++di; di < number.length; ++di) {
                if (number[di] == 'e') break;
                if (msd == '0') { --exponent_adjust; msd = number[di]; }
                else { mantissa += number[di]; }
            }
            for (++di; di < number.length; ++di) {
                if (number[di] == '-') exponent_sign = -1;
                else { exponent = exponent * 10 + ((number[di] as any) - ('0' as any)); }
            }
            exponent = exponent_sign * exponent + exponent_adjust;
            // adjust mantissa to precision
            let use_fixed = false;
            precision = (precision == -1 ? 6 : precision);
            if (lc == 'g') {
                /* ISO/IEC 9899-2011 :: §7.21.6.1:8
                   Let P equal the precision if nonzero, 6 if the precision is
                   omitted, or 1 if the precision is zero. Then, if a conversion
                   with style E would have an exponent of X:
                      — if P > X ≥ −4, the conversion is with style f (or F) and
                        precision P − (X + 1).
                      — otherwise, the conversion is with style e (or E) and
                        precision P − 1.
                */
                let P = (precision == 0 ? 1 : precision);
                if (P > exponent && exponent >= -4) {
                    use_fixed = true;
                    precision = P - (exponent + 1);
                } else {
                    precision = P - 1;
                }
                if (use_fixed) {
                    number = absolute.toFixed(precision);
                    if (precision == 0 && (flags & flag_hash_modifier)) number += '.';
                }
            }
            if (!use_fixed) {
                // precision includes msd in case of g
                let l = mantissa.length;
                if (l < precision) while (l++ < precision) mantissa += '0';
                else if (l > precision) {
                    number = Number(msd + '.' + mantissa).toFixed(precision);
                    // rounded number is either 10.000... or n.nnn...
                    //console.log(value, msd, mantissa, number);
                    if (number[1] != '.') { msd = 1; mantissa = number.substring(3); ++exponent; }
                    else { msd = number[0]; mantissa = number.substring(2); }
                }
                // stringize exponent making sure it has atleast two digits
                exponent_sign = (exponent < 0 ? '-' : '+');
                exponent = (exponent < 0 ? -exponent : exponent).toString();
                if (exponent.length == 1) exponent = '0' + exponent;
                exponent = exponent_sign + exponent;
                let decimal = ((flags & flag_hash_modifier) || mantissa ? '.' : '');
                number = msd + decimal + mantissa;
            }
            if (lc == 'g') {
                // remove trailing zeros and .
                if (!(flags & flag_hash_modifier) && (number.length > 1)) {
                    let last = number.length - 1;
                    while (number[last] == '0') --last;
                    if (number[last] == '.') --last;
                    number = number.substring(0, last + 1);
                }
            }
            if (!use_fixed) {
                let e = (c == lc ? 'e' : 'E');
                number += e + exponent;
            }
            let padding_length = width - sign.length - number.length;
            return add_padding_to_number(sign, number, padding_length, flags);
        }
    },

    printf_cstring(flags:number, width:number, precision:number, length_char:number, length:number, type:number, pointer:number) {

        if (length != 1) {
            throw 'unsupported char length in cstring: ' + length;
        }

        let si = pointer, string;
        if (precision >= 0) {
            //[TODO] test if precision is longer than zero terminated string length
            string = WASM.string(si, precision);
        } else {
            let view = WASM.ui8_rawmem();
            let max_si = view.byteLength;
            while (view[si] != 0 && si < max_si) ++si;
            string = WASM.string(pointer, si - pointer);
        }
        let padding_length = width - string.length;
        if (padding_length > 0) {
            let padding = ''; while (padding_length-- > 0) padding += ' ';
            string = ((flags & flag_left_align) ? string + padding : padding + string);
        }
        return handle_newline_and_update(string);
    },


    printf_char(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {
        let string = String.fromCharCode(value);
        let padding_length = width - string.length;
        if (padding_length > 0) {
            let padding = ''; while (padding_length-- > 0) padding += ' ';
            string = ((flags & flag_left_align) ? string + padding : padding + string);
        }
        return handle_newline_and_update(string);
    },

    printf_pointer(flags:number, width:number, precision:number, length_char:number, length:number, type:number, value:number) {

        let pointer = (value == 0 ? '(nil)' : '0x' + value.toString(16));
        let padding_length = width - pointer.length;
        if (padding_length > 0) {
            let padding = ''; while (padding_length-- > 0) padding += ' ';
            pointer = ((flags & flag_left_align) ? pointer + padding : padding + pointer);
        }
        printf_buffer += pointer;
        return pointer.length;
    }
});