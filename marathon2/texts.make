# Friday, September 9, 1994 6:40:21 PM  (alain)

# sorry for the lack of variables
# this encryptes the text files into temp files, uses stream edit to prepare the list of files for 
#   rez, then rezs them, then deletes the encrypted files
# there are three translate calls: 
#   the first one does a simple subsitution cipher to "encrypt" the text. Not very secure, but it does prevent casual inspection.
#   the second one eliminates the annoying ' that stream edit accidently puts in if i have files with spaces in them.
#   the third one translates some characters to " and ' because StreamEdit's print didn't want to print those, so i used special characters
# The reason i use the directory command is because it makes the streamedit command much simpler
# This expects the level files to be named Lnn≈ where nn is the level number, padded with a 
#   preceeding zero if it's a single digit.

#final
Texts ƒƒ {TextFiles}
	Directory texts
	Files L≈ | StreamEdit -d -e '/[Ll]([0-9]+)®1.(≈)®2/ print "Read ¡term¡ (1"®1 ", ™" ®2 "™) ™L" ®1 "." ®2 "™;"' | Translate "∂'" | Translate "¡™" "∂'∂"" | Rez -o texts.resource
	SetFile texts.resource -c 'RSED' -t 'RSRC'
	Directory ::
	Echo Done.

#demo
Texts.demo ƒƒ {TextFiles}
	Directory texts.demo
	Files L≈ | StreamEdit -d -e '/[Ll]([0-9]+)®1.(≈)®2/ print "Read ¡term¡ (1"®1 ", ™" ®2 "™) ™L" ®1 "." ®2 "™;"' | Translate "∂'" | Translate "¡™" "∂'∂"" | Rez -o texts.resource
	SetFile texts.resource -c 'RSED' -t 'RSRC'
	Directory ::
	Echo Done.

#Texts ƒƒ {TextFiles}
#	Set Echo 0
#	Directory texts
#	Echo Encrypting...
#	for filename in `Files L≈`
#		Catenate "{filename}" | Translate "A-MN-Z a-mn-z#" "N-ZA-Mn-z# a-m" > "E""{filename}"
#	end
#	Echo Rezzing...
#	Files EL≈ | StreamEdit -d -e '/E[Ll]([0-9]+)®1.(≈)®2/ print "Read ¡levl¡ (10"®1 ", ™" ®2 "™) ™EL" ®1 "." ®2 "™;"' | Translate "∂'" | Translate "¡™" "∂'∂"" | Rez -o texts.resource
#	SetFile texts.resource -c 'RSED' -t 'RSRC'
#	Delete EL≈
#	Directory ::
#	Echo Done.
