import ark_fbs

s = ark_fbs.Schema.from_fbs_text("namespace t; table A { x:int;y:int; } root_type A;")
b = s.json_to_binary('{"x":1,"y":2}')
print(s.binary_to_json(b))
