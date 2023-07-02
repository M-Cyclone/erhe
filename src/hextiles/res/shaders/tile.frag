in vec2       v_texcoord;
in vec4       v_color;
in flat uvec2 v_texture;

void main(void)
{
#if defined(ERHE_BINDLESS_TEXTURE)
    sampler2D s_texture = sampler2D(v_texture);
#endif

    vec4      t_color   = texture(s_texture, v_texcoord);

    out_color = v_color * t_color;
    //out_color = vec4(v_texcoord.xy, 0.0, 1.0);
}
