NodeManager::NodeManager(View* parent) : Panel(parent, Size{200_sx, ~0}) {
  setCollapsible().setVisible(false);
  nodeList.onChange([&] { eventChange(); });
}

auto NodeManager::show() -> void {
  refresh();
  setVisible(true);
}

auto NodeManager::hide() -> void {
  setVisible(false);
  root = {};
}

auto NodeManager::bind(higan::Node::Object root) -> void {
  this->root = root;
  nodeList.reset();
}

auto NodeManager::refresh() -> void {
  //save the currently selected node to try and reslect it after rebuilding the tree
  higan::Node::Object active;
  if(auto item = nodeList.selected()) active = item.attribute<higan::Node::Object>("node");

  nodeList.reset();
  for(auto& node : *root) refresh(node, 0);

  if(active) {
    //try and restore the previously selected node
    for(auto& item : nodeList.items()) {
      if(item.attribute<higan::Node::Object>("node") == active) {
        item.setSelected();
        break;
      }
    }
  }

  nodeList.resizeColumn().doChange();
}

auto NodeManager::refresh(higan::Node::Object node, uint depth) -> void {
  if(node->is<higan::Node::Input>()) return;
  if(node->is<higan::Node::Sprite>()) return;
  if(node->is<higan::Node::Component>() && !settings.interface.advancedMode) return;

  ListViewItem item{&nodeList};
  item.setAttribute<higan::Node::Object>("node", node);
  item.setAttribute<uint>("depth", depth);
  string name;
  for(uint n : range(depth)) name.append("   ");
  name.append(node->attribute("name") ? node->attribute("name") : node->name());
  if(auto setting = node->cast<higan::Node::Setting>()) {
    name.append(": ", setting->readValue());
    if(!setting->dynamic() && setting->readLatch() != setting->readValue()) {
      name.append(" (", setting->readLatch(), ")");
    }
  }
  item.setText(name);

  for(auto& node : *node) refresh(node, depth + 1);
}

auto NodeManager::refreshSettings() -> void {
  for(auto& item : nodeList.items()) {
    auto node = item.attribute<higan::Node::Object>("node");
    if(auto setting = node->cast<higan::Node::Setting>()) {
      auto depth = item.attribute<uint>("depth");
      string name;
      for(uint n : range(depth)) name.append("   ");
      name.append(node->attribute("name") ? node->attribute("name") : node->name());
      name.append(": ", setting->readValue());
      if(!setting->dynamic() && setting->readLatch() != setting->readValue()) {
        name.append(" (", setting->readLatch(), ")");
      }
      item.setText(name);
    }
  }
}

auto NodeManager::deselect() -> void {
  if(auto item = nodeList.selected()) {
    item.setSelected(false);
  }
}

auto NodeManager::eventChange() -> void {
  if(auto node = nodeList.selected().attribute<higan::Node::Object>("node")) {
    if(auto port = node->cast<higan::Node::Port>()) {
      portConnector.refresh(port);
      return programWindow.show(portConnector);
    }

    if(auto setting = node->cast<higan::Node::Setting>()) {
      settingEditor.refresh(setting);
      return programWindow.show(settingEditor);
    }

    if(auto inputs = node->find<higan::Node::Input>()) {
      if(inputs.first()->parent() == node) {
        inputMapper.refresh(node);
        return programWindow.show(inputMapper);
      }
    }
  }

  programWindow.show(home);
}
