#;
# install_zsh_and_oh_my_zsh()
# Installs zsh shell and oh-my-zsh enhancement package
# @param user: user to install bash-git-prompt-for
# @return error code
#"
USERNAME=vscode

function install_zsh_and_oh_my_zsh {
    
    # Uncomment en_US.UTF-8 for inclusion in generation
    sudo sed -i 's/^# *\(en_US.UTF-8\)/\1/' /etc/locale.gen
    
    # Generate locale
    sudo locale-gen
    
    # Export env vars
    echo "export LC_ALL=en_US.UTF-8" >> /home/${USERNAME}/.bashrc
    echo "export LANG=en_US.UTF-8" >> /home/${USERNAME}/.bashrc
    echo "export LANGUAGE=en_US.UTF-8" >> /home/${USERNAME}/.bashrc
    
    # Install "oh my zsh" --unattended
    sudo su ${USERNAME} -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh) \"\" --unattended"
    
    # Update default shell for current user to zsh
    sudo chsh -s $(which zsh) ${USERNAME}
    
    # Update default shell for root user to zsh
    sudo chsh -s $(which zsh)
    
    # Copy oh-my-zsh config beforehand
    # sudo su ${1} -c "cp /home/${1}/.oh-my-zsh/templates/zshrc.zsh-template /home/${1}/.zshrc"
    
    # Replace the default theme with `funky` theme
    sudo sed -i 's/^ZSH_THEME="devcontainers"/ZSH_THEME="refined"/' /home/${USERNAME}/.zshrc
    
    # Install oh-my-zsh plugins
    
    # Autosuggestions: https://github.com/zsh-users/zsh-autosuggestions
    # It suggests commands as you type based on history and completions.
    git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-/home/${USERNAME}/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
    
    # hacker-quotes git clone https://github.com/oldratlee/hacker-quotes
    git clone https://github.com/oldratlee/hacker-quotes.git ${ZSH_CUSTOM:-/home/${USERNAME}/.oh-my-zsh/custom}/plugins/hacker-quotes
    
    # ls: https://github.com/zpm-zsh/ls
    # Zsh plugin for ls. It improves the output of ls
    git clone https://github.com/zpm-zsh/ls.git ${ZSH_CUSTOM:-/home/${USERNAME}/.oh-my-zsh/custom}/plugins/ls
    
    # Enable plugins
    sudo sed -i 's/^plugins=(git)/plugins=(git git-auto-fetch zsh-autosuggestions ls hacker-quotes)/' /home/${USERNAME}/.zshrc
    
    return $?
}

function put_gdbinit_file {
    sudo su ${USERNAME} -c "cp .gdbinit /home/${USERNAME}/.gdbinit"
}

function init_conan_pkg_root {
    sudo mkdir -p /opt/conan-pkg-root && sudo chown -R ${USERNAME}:${USERNAME} /opt/conan-pkg-root
}



install_zsh_and_oh_my_zsh
put_gdbinit_file
init_conan_pkg_root

exit 0
