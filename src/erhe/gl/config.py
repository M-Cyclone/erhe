"""Configuration."""

EXTENSION_SUFFIXES = [
    '_3DFX',
    '_AMD',
    '_ANDROID',
    '_ANGLE',
    '_APPLE',
    '_ARB',
    '_ARM',
    '_ATI',
    '_AUTODES',
    '_DMP',
    '_EXT',
    '_HP',
    '_IBM',
    '_IMG',
    '_INGR',
    '_INTEL',
    '_KHR',
    '_MESA',
    '_MESAX',
    '_MTK',
    '_NV',
    '_NVX',
    '_OES',
    '_OML',
    '_OVR',
    '_PGI',
    '_QCOM',
    '_REND',
    '_SGI',
    '_SGIS',
    '_SGIX',
    '_SUN',
    '_VIV' ]
RESERVED_NAMES = ['and', 'bool', 'double', 'false', 'float', 'int',
                  'max', 'min', 'or', 'short', 'true', 'xor']
FUNCTION_SUFFIXES = {
    'ui64_v': '_ui64v',
    'ui64v':  '_ui64v',
    'Iuiv':   '_i_uiv',
    'i64_v':  '_i64v',
    'i64v':   '_i64v',
    '64iv':   '_i64v',
    '2x3fv':  '_2x3_fv',
    '2x4fv':  '_2x4_fv',
    '2x3dv':  '_2x3_dv',
    '2x4dv':  '_2x4_dv',
    '3x2fv':  '_3x2_fv',
    '4x2fv':  '_4x2_fv',
    '3x2dv':  '_3x2_dv',
    '4x2dv':  '_4x2_dv',
    '3x4fv':  '_3x4_fv',
    '4x3fv':  '_4x3_fv',
    '3x4dv':  '_3x4_dv',
    '4x3dv':  '_4x3_dv',
    'Iiv':    '_i_iv',
    'i64':    '_i64',
    '1iv':    '_1iv',
    '2iv':    '_2iv',
    '3iv':    '_3iv',
    '4iv':    '_4iv',
    '1fv':    '_1fv',
    '2fv':    '_2fv',
    '3fv':    '_3fv',
    '4fv':    '_4fv',
    '1dv':    '_1dv',
    '2dv':    '_2dv',
    '3dv':    '_3dv',
    '4dv':    '_4dv',
    'uiv':    '_uiv',
    'f_i':    '_f_i',
    '_iv':    '_iv',
    'i_v':    '_iv',
    'iv':     '_iv',
    'fv':     '_fv',
    '1i':     '_1i',
    '2i':     '_2i',
    '3i':     '_3i',
    '4i':     '_4i',
    '1s':     '_1s',
    '2s':     '_2s',
    '3s':     '_3s',
    '4s':     '_4s',
    '1f':     '_1f',
    '2f':     '_2f',
    '3f':     '_3f',
    '4f':     '_4f',
    '1d':     '_1d',
    '2d':     '_2d',
    '3d':     '_3d',
    '4d':     '_4d',
    'i':      '_i',
    'f':      '_f',
    '_v':     '_v',
    'v':      '_v'}
HANDLE_TYPES = ['GLsync']
EXCLUDED_ENUMS = ['GL_INVALID_INDEX', 'GL_TIMEOUT_IGNORED']
EXTRA_ENUM_GROUPS = {
    'ActiveAttribType': [
        'GL_UNSIGNED_INT',
        'GL_FLOAT',
        'GL_DOUBLE',
        'GL_FLOAT_VEC2',
        'GL_FLOAT_VEC3',
        'GL_FLOAT_VEC4',
        'GL_INT_VEC2',
        'GL_INT_VEC3',
        'GL_INT_VEC4',
        'GL_FLOAT_MAT2',
        'GL_FLOAT_MAT3',
        'GL_FLOAT_MAT4',
        'GL_UNSIGNED_INT_VEC2',
        'GL_UNSIGNED_INT_VEC3',
        'GL_UNSIGNED_INT_VEC4',
        'GL_DOUBLE_MAT2',
        'GL_DOUBLE_MAT3',
        'GL_DOUBLE_MAT4',
        'GL_DOUBLE_MAT2x3',
        'GL_DOUBLE_MAT2x4',
        'GL_DOUBLE_MAT3x2',
        'GL_DOUBLE_MAT3x4',
        'GL_DOUBLE_MAT4x2',
        'GL_DOUBLE_MAT4x3',
        'GL_DOUBLE_VEC2',
        'GL_DOUBLE_VEC3',
        'GL_DOUBLE_VEC4'],
    'FragmentShaderOutputType': [
        'GL_INT',
        'GL_INT_VEC2',
        'GL_INT_VEC3',
        'GL_INT_VEC4',
        'GL_UNSIGNED_INT',
        'GL_UNSIGNED_INT_VEC2',
        'GL_UNSIGNED_INT_VEC3',
        'GL_UNSIGNED_INT_VEC4',
        'GL_FLOAT',
        'GL_FLOAT_VEC2',
        'GL_FLOAT_VEC3',
        'GL_FLOAT_VEC4',
        'GL_DOUBLE',
        'GL_DOUBLE_VEC2',
        'GL_DOUBLE_VEC3',
        'GL_DOUBLE_VEC4'],
    'TextureCompareMode': [
        'GL_NONE',
        'GL_COMPARE_REF_TO_TEXTURE'],
    'TextureCompareFunc': [
        'GL_LEQUAL',
        'GL_GEQUAL',
        'GL_LESS',
        'GL_GREATER',
        'GL_EQUAL',
        'GL_NOTEQUAL',
        'GL_ALWAYS',
        'GL_NEVER'],
    'TextureMagFilter': [
        'GL_NEAREST',
        'GL_LINEAR'],
    'TextureMinFilter': [
        'GL_NEAREST',
        'GL_LINEAR',
        'GL_NEAREST_MIPMAP_NEAREST',
        'GL_LINEAR_MIPMAP_NEAREST',
        'GL_NEAREST_MIPMAP_LINEAR',
        'GL_LINEAR_MIPMAP_LINEAR'],
    'TextureStencilMode': [
        'GL_DEPTH_COMPONENT',
        'GL_STENCIL_INDEX'],
    'TextureTarget': [
        'GL_TEXTURE_BUFFER'],
    'TextureWrapMode': [
        'GL_CLAMP_TO_EDGE',
        'GL_CLAMP_TO_BORDER',
        'GL_REPEAT',
        'GL_MIRRORED_REPEAT',
        'GL_MIRROR_CLAMP_TO_EDGE'],
    'BlendEquationModeEXT': [
        'GL_FUNC_ADD',
        'GL_FUNC_SUBTRACT',
        'GL_FUNC_REVERSE_SUBTRACT',
        'GL_MIN',
        'GL_MAX'],
    'TextureSwizzle': [
        'GL_RED',
        'GL_GREEN',
        'GL_BLUE',
        'GL_ALPHA',
        'GL_ZERO',
        'GL_ONE',
    ],
    'SamplerParameterF': [
        'GL_TEXTURE_LOD_BIAS',
    ]}
