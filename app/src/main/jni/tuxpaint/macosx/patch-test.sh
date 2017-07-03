#Test en changeant les chemins orginaux des bibliotheques dynamiques
#sudo mv /usr/local/lib /usr/local/_lib
#sudo mv /opt/local/lib /opt/local/_lib
sudo mv /Library/Frameworks/SDL.framework /Library/Frameworks/_SDL.framework
sudo mv /Library/Frameworks/SDL_image.framework /Library/Frameworks/_SDL_image.framework
sudo mv /Library/Frameworks/SDL_mixer.framework /Library/Frameworks/_SDL_mixer.framework
sudo mv /Library/Frameworks/SDL_ttf.framework /Library/Frameworks/_SDL_ttf.framework
"$1"
sudo mv /Library/Frameworks/_SDL.framework /Library/Frameworks/SDL.framework
sudo mv /Library/Frameworks/_SDL_image.framework /Library/Frameworks/SDL_image.framework
sudo mv /Library/Frameworks/_SDL_mixer.framework /Library/Frameworks/SDL_mixer.framework
sudo mv /Library/Frameworks/_SDL_ttf.framework /Library/Frameworks/SDL_ttf.framework
#sudo mv /opt/local/_lib /opt/local/lib
#sudo mv /usr/local/_lib /usr/local/lib
