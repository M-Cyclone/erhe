//out      vec3  v_texcoord;
out      vec2  v_texcoord;
out      vec4  v_color;

#if defined(ERHE_BINDLESS_TEXTURE)
out flat uvec2 v_texture;
#endif

void main()
{
    //const vec2[] corner_texcoords = vec2[4]
    //(
    //    vec2(0.0, 0.0),
    //    vec2(1.0, 0.0),
    //    vec2(1.0, 1.0),
    //    vec2(0.0, 1.0)
    //);
    gl_Position = projection.clip_from_window * vec4(a_position, -0.5, 1.0);

#if defined(ERHE_BINDLESS_TEXTURE)
    v_texture   = projection.texture;
#endif

    //v_texcoord  = vec3(corner_texcoords[a_tile_corner.y], a_tile_corner.x);
    v_texcoord  = a_texcoord;
    v_color     = a_color;
}

